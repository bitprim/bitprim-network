/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/proxy.hpp>

#define BOOST_BIND_NO_PLACEHOLDERS

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/const_buffer.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/socket.hpp>

namespace libbitcoin {
namespace network {

#define NAME "proxy"

using namespace message;
using namespace std::placeholders;

// payload_buffer_ sizing assumes monotonically increasing size by version.
proxy::proxy(threadpool& pool, socket::ptr socket, uint32_t protocol_magic,
    uint32_t protocol_maximum)
  : protocol_magic_(protocol_magic),
    authority_(socket->get_authority()),
    heading_buffer_(heading::maximum_size()),
    payload_buffer_(heading::maximum_payload_size(protocol_maximum)),
    socket_(socket),
    stopped_(true),
    version_(protocol_maximum),
    message_subscriber_(pool),
    stop_subscriber_(std::make_shared<stop_subscriber>(pool, NAME))
{
}

proxy::~proxy()
{
    BITCOIN_ASSERT_MSG(stopped(), "The channel was not stopped.");
}

// Properties.
// ----------------------------------------------------------------------------

const config::authority& proxy::authority() const
{
    return authority_;
}

uint32_t proxy::negotiated_version() const
{
    return version_.load();
}

void proxy::set_negotiated_version(uint32_t value)
{
    version_.store(value);
}

// Start sequence.
// ----------------------------------------------------------------------------

void proxy::start(result_handler handler)
{
    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    stopped_ = false;
    stop_subscriber_->start();
    message_subscriber_.start();

    // Allow for subscription before first read, so no messages are missed.
    handler(error::success);

    // Start the read cycle.
    read_heading();
}

// Stop subscription.
// ----------------------------------------------------------------------------

void proxy::subscribe_stop(result_handler handler)
{
    stop_subscriber_->subscribe(handler, error::channel_stopped);
}

// Read cycle (read continues until stop).
// ----------------------------------------------------------------------------

void proxy::read_heading()
{
    if (stopped())
        return;

    // The heading buffer is protected by ordering, not the critial section.

    // Critical Section (external)
    ///////////////////////////////////////////////////////////////////////////
    const auto socket = socket_->get_socket();

    using namespace boost::asio;
    async_read(socket->get(), buffer(heading_buffer_, heading_buffer_.size()),
        std::bind(&proxy::handle_read_heading,
            shared_from_this(), _1, _2));
    ///////////////////////////////////////////////////////////////////////////
}

void proxy::handle_read_heading(const boost_code& ec, size_t)
{
    if (stopped())
        return;

    // TODO: verify client quick disconnect.
    if (ec)
    {
        log::debug(LOG_NETWORK)
            << "Heading read failure [" << authority() << "] "
            << code(error::boost_to_error_code(ec)).message();
        stop(ec);
        return;
    }

    const auto head = heading::factory_from_data(heading_buffer_);

    if (!head.is_valid())
    {
        log::warning(LOG_NETWORK) 
            << "Invalid heading from [" << authority() << "]";
        stop(error::bad_stream);
        return;
    }

    if (head.magic != protocol_magic_)
    {
        log::warning(LOG_NETWORK)
            << "Invalid heading magic (" << head.magic << ") from ["
            << authority() << "]";
        stop(error::bad_stream);
        return;
    }

    if (head.payload_size > payload_buffer_.capacity())
    {
        log::warning(LOG_NETWORK)
            << "Oversized payload indicated by " << head.command
            << " heading from [" << authority() << "] ("
            << head.payload_size << " bytes)";
        stop(error::bad_stream);
        return;
    }

    read_payload(head);
    handle_activity();
}

void proxy::read_payload(const heading& head)
{
    if (stopped())
        return;

    // This does not cause a reallocation.
    payload_buffer_.resize(head.payload_size);

    // The payload buffer is protected by ordering, not the critial section.

    // Critical Section (external)
    ///////////////////////////////////////////////////////////////////////////
    const auto socket = socket_->get_socket();

    using namespace boost::asio;
    async_read(socket->get(), buffer(payload_buffer_, head.payload_size),
        std::bind(&proxy::handle_read_payload,
            shared_from_this(), _1, _2, head));
    ///////////////////////////////////////////////////////////////////////////
}

void proxy::handle_read_payload(const boost_code& ec, size_t payload_size,
    const heading& head)
{
    if (stopped())
        return;

    // TODO: verify client quick disconnect.
    if (ec)
    {
        log::debug(LOG_NETWORK)
            << "Payload read failure [" << authority() << "] "
            << code(error::boost_to_error_code(ec)).message();
        stop(ec);
        return;
    }

    if (head.checksum != bitcoin_checksum(payload_buffer_))
    {
        log::warning(LOG_NETWORK) 
            << "Invalid " << head.command << " payload from [" << authority()
            << "] bad checksum.";
        stop(error::bad_stream);
        return;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    // TODO: we aren't getting a stream benefit if we read the full payload
    // before parsing the message. Should just make this a message parse.
    ///////////////////////////////////////////////////////////////////////////

    // Notify subscribers of the new message.
    payload_source source(payload_buffer_);
    payload_stream istream(source);
    const auto code = message_subscriber_.load(head.type(), version_, istream);
    const auto consumed = istream.peek() == std::istream::traits_type::eof();

    // For finding agents of bad versions.
    ////const auto agent =
    ////    ((!code && consumed) || head.command != version::command) ? "" : " " +
    ////        std::string(payload_buffer_.begin(), payload_buffer_.end());

    if (code)
    {
        log::warning(LOG_NETWORK)
            << "Invalid " << head.command << " payload from [" << authority()
            << "] " << code.message();
        stop(code);
        return;
    }

    if (!consumed)
    {
        log::warning(LOG_NETWORK)
            << "Invalid " << head.command << " payload from [" << authority()
            << "] trailing bytes.";
        stop(error::bad_stream);
        return;
    }

    log::debug(LOG_NETWORK)
        << "Valid " << head.command << " payload from [" << authority()
        << "] (" << payload_size << " bytes)";

    handle_activity();
    read_heading();
}

// Message send sequence.
// ----------------------------------------------------------------------------

void proxy::do_send(const std::string& command, const_buffer buffer,
    result_handler handler)
{
    if (stopped())
    {
        handler(error::channel_stopped);
        return;
    }

    log::debug(LOG_NETWORK)
        << "Sending " << command << " to [" << authority() << "] ("
        << buffer.size() << " bytes)";

    // Critical Section (protect socket)
    ///////////////////////////////////////////////////////////////////////////
    // The socket is locked until async_write returns.
    const auto socket = socket_->get_socket();

    // The shared buffer is kept in scope until the handler is invoked.
    using namespace boost::asio;
    async_write(socket->get(), buffer,
        std::bind(&proxy::handle_send,
            shared_from_this(), _1, buffer, handler));
    ///////////////////////////////////////////////////////////////////////////
}

void proxy::handle_send(const boost_code& ec, const_buffer buffer,
    result_handler handler)
{
    const auto error = code(error::boost_to_error_code(ec));

    if (error)
        log::debug(LOG_NETWORK)
            << "Failure sending " << buffer.size() << " byte message to ["
            << authority() << "] " << error.message();

    handler(error);
}

// Stop sequence.
// ----------------------------------------------------------------------------

// This is not short-circuited by a stop test because we need to ensure it
// completes at least once before invoking the handler. That would require a
// lock be taken around the entire section, which poses a deadlock risk.
// Instead this is thread safe and idempotent, allowing it to be unguarded.
void proxy::stop(const code& ec)
{
    BITCOIN_ASSERT_MSG(ec, "The stop code must be an error code.");

    stopped_ = true;

    // Prevent subscription after stop.
    message_subscriber_.stop();
    message_subscriber_.broadcast(error::channel_stopped);

    // Prevent subscription after stop.
    stop_subscriber_->stop();
    stop_subscriber_->relay(ec);

    // Give channel opportunity to terminate timers.
    handle_stopping();

    // The socket_ is internally guarded against concurrent use.
    socket_->close();
}

void proxy::stop(const boost_code& ec)
{
    stop(error::boost_to_error_code(ec));
}

bool proxy::stopped() const
{
    return stopped_;
}

} // namespace network
} // namespace libbitcoin
