/*
 * SatHelperException.h
 *
 *  Created on: 04/11/2016
 *      Author: Lucas Teske
 */
#pragma once

#include <exception>
#include <string>

namespace sathelper
{
    class SatHelperException : public std::exception
    {
    public:
        SatHelperException() : msg_("") {}

        SatHelperException(const char *message) : msg_(message) {}

        SatHelperException(const std::string &message) : msg_(message) {}

        virtual ~SatHelperException() throw() {}

        virtual const char *what() const throw() { return msg_.c_str(); }

        virtual const std::string reason() { return msg_; }

    protected:
        // Error message.
        std::string msg_;
    };
} // namespace sathelper