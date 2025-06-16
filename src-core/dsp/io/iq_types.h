#pragma once

#include <string>

namespace satdump
{
    namespace ndsp
    {
        enum IQTypeEnum
        {
            CF32,
            CS32,
            CS16,
            CS8,
            CU8,
            WAV16,
            // #ifdef BUILD_ZIQ
            //             ZIQ,
            // #endif
            // #ifdef BUILD_ZIQ2
            //             ZIQ2,
            // #endif
        };

        class IQType
        {
        private:
            IQTypeEnum type;
            void from_string(const std::string &s);

        public:
            IQType() { type = CF32; }

            IQType(const IQTypeEnum &e) { type = e; }

            IQType(const std::string &s) { from_string(s); }

            IQType(const char *s) { from_string(std::string(s)); }

            inline operator IQTypeEnum() const { return type; }

            inline IQType &operator=(const IQTypeEnum &e)
            {
                type = e;
                return *this;
            }

            inline IQType &operator=(const std::string &s)
            {
                from_string(s);
                return *this;
            }

            inline IQType &operator=(const char *s)
            {
                from_string(std::string(s));
                return *this;
            }

            operator std::string() const;

            bool draw_combo(const char *label = "Format");

            // #if defined(BUILD_ZIQ) || defined(BUILD_ZIQ2)
            //             int ziq_depth = 8;
            //             IQType(IQTypeEnum e, int d)
            //             {
            //                 type = e;
            //                 ziq_depth = d;
            //             }

            //             friend struct std::less<IQType>;
            // #endif
        };
    } // namespace ndsp
} // namespace satdump