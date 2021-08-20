/**
 * Copyright (c) 2018 Cornell University.
 *
 * Author: Ted Yin <tederminant@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SALTICIDAE_UTIL_H
#define _SALTICIDAE_UTIL_H

#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include "salticidae/config.h"

typedef struct SalticidaeCError {
    int code;
    int oscode;
} SalticidaeCError;

#ifdef __cplusplus
#include <stdexcept>
extern "C" {
#endif

SalticidaeCError salticidae_cerror_normal();
SalticidaeCError salticidae_cerror_unknown();
const char *salticidae_strerror(int code);

#define SALTICIDAE_CERROR_TRY(cerror) try {  (*(cerror)) = salticidae_cerror_normal();
#define SALTICIDAE_CERROR_CATCH(cerror) \
    } catch (const SalticidaeError &err) { \
        *cerror = err.get_cerr(); \
    } catch (const std::exception &err) { \
        *cerror = salticidae_cerror_unknown(); \
    }

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>
#include <exception>
#include <cstdarg>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <functional>
#include "salticidae/ref.h"

namespace salticidae {

void sec2tv(double t, struct timeval &tv);
double gen_rand_timeout(double base_timeout, double alpha = 0.5);

std::string trim(const std::string &s,
                const std::string &space = "\t\r\n ");
std::vector<std::string> split(const std::string &s, const std::string &delim);
std::vector<std::string> trim_all(const std::vector<std::string> &ss);
std::string vstringprintf(const char *fmt, va_list ap);
std::string stringprintf(const char *fmt, ...);

template<typename SerialType>
inline std::string get_hex10(const SerialType &x) {
    return get_hex(x).substr(0, 10);
}

enum SalticidaeErrorCode {
    SALTI_NORMAL,
    SALTI_ERROR_GENERIC,
    SALTI_ERROR_ACCEPT,
    SALTI_ERROR_LISTEN,
    SALTI_ERROR_CONNECT,
    SALTI_ERROR_PEER_ALREADY_EXISTS,
    SALTI_ERROR_PEER_NOT_EXIST,
    SALTI_ERROR_PEER_NOT_READY,
    SALTI_ERROR_PEER_NOT_MATCH,
    SALTI_ERROR_CLIENT_NOT_EXIST,
    SALTI_ERROR_NETADDR_INVALID,
    SALTI_ERROR_OPTVAL_INVALID,
    SALTI_ERROR_OPTNAME_ALREADY_EXISTS,
    SALTI_ERROR_OPT_UNKNOWN_ACTION,
    SALTI_ERROR_CONFIG_LINE_TOO_LONG,
    SALTI_ERROR_OPT_INVALID,
    SALTI_ERROR_TLS_LOAD_CERT,
    SALTI_ERROR_TLS_LOAD_KEY,
    SALTI_ERROR_TLS_GENERIC,
    SALTI_ERROR_TLS_X509,
    SALTI_ERROR_TLS_KEY,
    SALTI_ERROR_TLS_KEY_NOT_MATCH,
    SALTI_ERROR_TLS_NO_PEER_CERT,
    SALTI_ERROR_FD,
    SALTI_ERROR_LIBUV_INIT,
    SALTI_ERROR_LIBUV_START,
    SALTI_ERROR_RAND_SOURCE,
    SALTI_ERROR_CONN_NOT_READY,
    SALTI_ERROR_NOT_AVAIL,
    SALTI_ERROR_UNKNOWN,
    SALTI_ERROR_CONN_OVERSIZED_MSG
};

extern const char *SALTICIDAE_ERROR_STRINGS[];

class SalticidaeError: public std::exception {
    std::string msg;
    int code;
    int oscode;

    public:
    SalticidaeError() : code(SALTI_NORMAL) {}

    template<typename... Args>
    SalticidaeError(const std::string &fmt, Args... args): code(SALTI_ERROR_GENERIC) {
        msg = stringprintf(fmt.c_str(), args...);
    }

    SalticidaeError(int code, int oscode = 0): code(code), oscode(oscode) {
        if (oscode)
            msg = stringprintf("%s: %s", SALTICIDAE_ERROR_STRINGS[code], strerror(oscode));
        else
            msg = SALTICIDAE_ERROR_STRINGS[code];
    }

    operator std::string() const { return msg; }
    const char *what() const throw() override { return msg.c_str(); }
    int get_code() const { return code; }
    int get_oscode() const { return oscode; }
    SalticidaeCError get_cerr() const {
        SalticidaeCError res;
        res.code = code;
        res.oscode = oscode;
        return res;
    }
};


struct ConnPoolError: public SalticidaeError {
    using SalticidaeError::SalticidaeError;
};

class MsgNetworkError: public ConnPoolError {
    using ConnPoolError::ConnPoolError;
};

class PeerNetworkError: public MsgNetworkError {
    using MsgNetworkError::MsgNetworkError;
};

class ClientNetworkError: public MsgNetworkError {
    using MsgNetworkError::MsgNetworkError;
};

extern const char *TTY_COLOR_RED;
extern const char *TTY_COLOR_GREEN;
extern const char *TTY_COLOR_YELLOW;
extern const char *TTY_COLOR_BLUE;
extern const char *TTY_COLOR_MAGENTA;
extern const char *TTY_COLOR_CYAN;
extern const char *TTY_COLOR_RESET;

class Logger {
    const char *color_info;
    const char *color_debug;
    const char *color_warning;
    const char *color_error;
    protected:
    int output;
    bool opened;
    const char *prefix;
    void write(const char *tag, const char *color,
                const char *fmt, va_list ap);
    void set_color();

    public:
    Logger(const char *prefix, int fd = 2):
        output(fd), opened(false), prefix(prefix) { set_color(); }
    Logger(const char *prefix, const char *filename):
        opened(true), prefix(prefix) {
        if ((output = open(filename, O_CREAT | O_WRONLY)) == -1)
            throw SalticidaeError("logger cannot open file %s", filename);
        set_color();
    }

    ~Logger() { if (opened) close(output); }

    void info(const char *fmt, ...);
    void debug(const char *fmt, ...);
    void warning(const char *fmt, ...);
    void error(const char *fmt, ...);
    bool is_tty() { return isatty(output); }
};

extern Logger logger;

#ifdef SALTICIDAE_DEBUG_LOG
#define SALTICIDAE_NORMAL_LOG
#define SALTICIDAE_ENABLE_LOG_DEBUG
#endif

#ifdef SALTICIDAE_NORMAL_LOG
#define SALTICIDAE_ENABLE_LOG_INFO
#define SALTICIDAE_ENABLE_LOG_WARN
#endif

#ifdef SALTICIDAE_ENABLE_LOG_INFO
#define SALTICIDAE_LOG_INFO(...) salticidae::logger.info(__VA_ARGS__)
#else
#define SALTICIDAE_LOG_INFO(...) ((void)0)
#endif

#ifdef SALTICIDAE_ENABLE_LOG_DEBUG
#define SALTICIDAE_LOG_DEBUG(...) salticidae::logger.debug(__VA_ARGS__)
#else
#define SALTICIDAE_LOG_DEBUG(...) ((void)0)
#endif

#ifdef SALTICIDAE_ENABLE_LOG_WARN
#define SALTICIDAE_LOG_WARN(...) salticidae::logger.warning(__VA_ARGS__)
#else
#define SALTICIDAE_LOG_WARN(...) ((void)0)
#endif

#define SALTICIDAE_LOG_ERROR(...) salticidae::logger.error(__VA_ARGS__)

class ElapsedTime {
    struct timeval t0;
    clock_t cpu_t0;
    public:
    double elapsed_sec;
    double cpu_elapsed_sec;
    void start();
    void stop(bool show_info = false);
};

class Config {
    public:
    enum Action {
        SWITCH_ON,
        SET_VAL,
        APPEND
    };

    class OptVal {
        public:
        virtual void switch_on() {
            throw SalticidaeError("undefined OptVal behavior: set_val");
        }
        
        virtual void set_val(const std::string &) {
            throw SalticidaeError("undefined OptVal behavior: set_val");
        }
        
        virtual void append(const std::string &) {
            throw SalticidaeError("undefined OptVal behavior: append");
        }

        virtual ~OptVal() = default;
    };

    using optval_t = RcObj<OptVal>;

    class OptValFlag: public OptVal {
        bool val;
        public:
        template<typename... Args>
        static RcObj<OptValFlag> create(Args... args) {
            return new OptValFlag(args...);
        }
        OptValFlag() = default;
        OptValFlag(bool val): val(val) {}
        void switch_on() override { val = true; }
        bool &get() { return val; }
    };

    class OptValStr: public OptVal {
        std::string val;
        public:
        template<typename... Args>
        static RcObj<OptValStr> create(Args... args) {
            return new OptValStr(args...);
        }
        OptValStr() = default;
        OptValStr(const std::string &val): val(val) {}
        void set_val(const std::string &strval) override {
            val = strval;
        }
        std::string &get() { return val; }
    };

    class OptValInt: public OptVal {
        int val;
        public:
        template<typename... Args>
        static RcObj<OptValInt> create(Args... args) {
            return new OptValInt(args...);
        }
        OptValInt() = default;
        OptValInt(int val): val(val) {}
        void set_val(const std::string &strval) override {
            size_t idx;
            try {
                val = stoi(strval, &idx);
            } catch (std::invalid_argument &) {
                throw SalticidaeError(SALTI_ERROR_OPTVAL_INVALID);
            }
        }
        int &get() { return val; }
    };

    class OptValDouble: public OptVal {
        double val;
        public:
        template<typename... Args>
        static RcObj<OptValDouble> create(Args... args) {
            return new OptValDouble(args...);
        }
        OptValDouble() = default;
        OptValDouble(double val): val(val) {}
        void set_val(const std::string &strval) override {
            size_t idx;
            try {
                val = stod(strval, &idx);
            } catch (std::invalid_argument &) {
                throw SalticidaeError(SALTI_ERROR_OPTVAL_INVALID);
            }
        }
        double &get() { return val; }
    };

    class OptValStrVec: public OptVal {
        using strvec_t = std::vector<std::string>;
        strvec_t val;
        public:
        template<typename... Args>
        static RcObj<OptValStrVec> create(Args... args) {
            return new OptValStrVec(args...);
        }
        OptValStrVec() = default;
        OptValStrVec(const strvec_t &val): val(val) {}
        void append(const std::string &strval) override {
            val.push_back(strval);
        }
        strvec_t &get() { return val; }
    };

    private:
    class OptValConf: public OptVal {
        Config *config;
        public:
        template<typename... Args>
        static RcObj<OptValConf> create(Args... args) {
            return new OptValConf(args...);
        }
        OptValConf(Config *config): config(config) {}
        void set_val(const std::string &fname) override {
            if (config->load(fname))
                SALTICIDAE_LOG_INFO("loading extra configuration from %s", fname.c_str());
            else
                SALTICIDAE_LOG_INFO("configuration file %s not found", fname.c_str());
        }
        std::string &get() = delete;
    };

    struct Opt {
        std::string optname;
        std::string optdoc;
        optval_t optval;
        Action action;
        struct option opt;
        char short_opt;
        Opt(const std::string &optname, const std::string &optdoc,
            const optval_t &optval, Action action,
            char short_opt,
            int idx);
        Opt(Opt &&other):
                optname(std::move(other.optname)),
                optdoc(std::move(other.optdoc)),
                optval(std::move(other.optval)),
                action(other.action),
                opt(other.opt),
                short_opt(other.short_opt) {
            opt.name = this->optname.c_str();
        }
    };

    std::unordered_map<std::string, Opt *> conf;
    std::vector<BoxObj<Opt>> opts;
    std::string conf_fname;
    RcObj<OptValConf> opt_val_conf;

    void update(const std::string &optname, const char *optval);
    void update(Opt &opt, const char *optval);

    public:
    Config() {}
    Config(const std::string &conf_fname):
            conf_fname(conf_fname),
            opt_val_conf(OptValConf::create(this)) {
        add_opt("conf", opt_val_conf, SET_VAL, 'c', "load options from a file");
    }
    
    ~Config() {}

    void add_opt(const std::string &optname, const optval_t &optval, Action action,
                char short_opt = -1,
                const std::string &optdoc = "");
    bool load(const std::string &fname);
    size_t parse(int argc, char **argv);
    void print_help(FILE *output = stderr);
};

}
#endif

#endif
