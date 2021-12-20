#ifndef SOCA_ERROR_CODE_HPP__
#define SOCA_ERROR_CODE_HPP__

namespace Soca{

enum class errc : int{
	//General
	insufficient_buffer		= 10,
	invalid_data,
	//message
	code_invalid 			= 20,
	invalid_token_length,
	message_too_small,
	version_invalid,
	type_invalid,
	empty_format_error,
	//Options
	option_invalid 			= 30,
	option_out_of_order,
	option_repeated,
	option_parse_error,
	option_not_found,
	//payload
	payload_no_marker		= 40,
	//socket
	socket_error			= 50,
	endpoint_error,
	socket_receive,
	socket_send,
	socket_bind,
	//transmission
	transaction_ocupied		= 60,
	no_free_slots,
	buffer_empty,
	request_not_supported
};

struct Error {
	int err_ = 0;

	Error(){}
	Error(const Error&) = default;

	int value() const noexcept;
	const char* message() const noexcept;
	static const char* message(int error);

	void clear() noexcept{ err_ = 0; }

	operator bool() const;
	inline bool operator==(Error const& rhs)
	{
		return err_ == rhs.err_;
	}

	inline bool operator!=(Error const& rhs)
	{
		return !(*this == rhs);
	}

	inline Error& operator=(Error const& other) noexcept
	{
		err_ = other.err_;
		return *this;
	}

	inline Error& operator=(errc const& error) noexcept
	{
		err_ = static_cast<int>(error);
		return *this;
	}

	inline bool operator==(errc const& error) noexcept
	{
		return err_ == static_cast<int>(error);
	}

	inline bool operator!=(errc const& error) noexcept
	{
		return !(*this == error);
	}
};

}//Soca

#endif /* SOCA_ERROR_CODE_HPP__ */
