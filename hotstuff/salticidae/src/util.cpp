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

#include <cstdarg>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <sys/time.h>

#include "salticidae/util.h"

namespace salticidae {

const char *SALTICIDAE_ERROR_STRINGS[] = {
    "",
    "generic",
    "unable to accept",
    "unable to listen",
    "unable to connect",
    "peer already exists",
    "peer does not exist",
    "peer is not ready",
    "peer id does not match the record",
    "client does not exist",
    "invalid NetAddr format",
    "invalid OptVal format",
    "option name already exists",
    "unknown action",
    "configuration file line too long",
    "invalid option format",
    "unable to load cert",
    "unable to load key",
    "tls generic error",
    "x509 cert error",
    "EVP_PKEY error",
    "tls key does not match the cert",
    "tls fail to get peer cert",
    "fd error",
    "libuv init failed",
    "libuv start failed",
    "rand source is not available, try again",
    "connection is not ready",
    "operation not available",
    "unknown error",
    "oversized message",
};

const char *TTY_COLOR_RED = "\x1b[31m";
const char *TTY_COLOR_GREEN = "\x1b[32m";
const char *TTY_COLOR_YELLOW = "\x1b[33m";
const char *TTY_COLOR_BLUE = "\x1b[34m";
const char *TTY_COLOR_MAGENTA = "\x1b[35m";
const char *TTY_COLOR_CYAN = "\x1b[36m";
const char *TTY_COLOR_RESET = "\x1b[0m";

extern "C" {

SalticidaeCError salticidae_cerror_normal() {
    SalticidaeCError res;
    res.code = SALTI_NORMAL;
    res.oscode = 0;
    return res;
}

SalticidaeCError salticidae_cerror_unknown() {
    SalticidaeCError res;
    res.code = SALTI_ERROR_UNKNOWN;
    res.oscode = 0;
    return res;
}

const char *salticidae_strerror(int code) {
    if (code < 0 ||
        code >= (int)(sizeof(SALTICIDAE_ERROR_STRINGS) / sizeof(const char *)))
        return "invalid";
    return SALTICIDAE_ERROR_STRINGS[code];
}

}

void sec2tv(double t, struct timeval &tv) {
    tv.tv_sec = trunc(t);
    tv.tv_usec = trunc((t - tv.tv_sec) * 1e6);
}

double gen_rand_timeout(double base_timeout, double alpha) {
    return base_timeout + rand() / (double)RAND_MAX * alpha * base_timeout;
}

std::string vstringprintf(const char *fmt, va_list _ap) {
    int guessed_size = 1024;
    std::string buff;
    va_list ap;
    va_copy(ap, _ap);
    buff.resize(guessed_size);
    int nwrote = vsnprintf(&buff[0], guessed_size, fmt, _ap);
    if (nwrote < 0) buff = "";
    else
    {
        buff.resize(nwrote);
        if (nwrote > guessed_size)
        {
            if (vsnprintf(&buff[0], nwrote, fmt, ap) != nwrote)
                buff = "";
        }
    }
    va_end(ap);
    return buff;
}

std::string stringprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    auto ret = vstringprintf(fmt, ap);
    va_end(ap);
    return ret;
}

const std::string get_current_datetime() {
    /* credit: http://stackoverflow.com/a/41381479/544806 */
    char fmt[64], buf[64];
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm *tmp = localtime(&tv.tv_sec);
    strftime(fmt, sizeof fmt, "%Y-%m-%d %H:%M:%S.%%06u", tmp);
    snprintf(buf, sizeof buf, fmt, tv.tv_usec);
    return std::string(buf);
}

void Logger::set_color() {
    if (is_tty())
    {
        color_info = TTY_COLOR_GREEN;
        color_debug = TTY_COLOR_BLUE;
        color_warning = TTY_COLOR_YELLOW;
        color_error = TTY_COLOR_RED;
    }
    else
    {
        color_info = color_debug = color_warning = color_error = nullptr;
    }
}

void Logger::write(const char *tag, const char *color,
                    const char *fmt, va_list ap) {
    std::string buff = color ? color : "";
    buff += stringprintf("%s [%s %s] ", get_current_datetime().c_str(), prefix, tag);
    if (color) buff += TTY_COLOR_RESET;
    buff += vstringprintf(fmt, ap);
    buff.push_back('\n');
    ::write(output, &buff[0], buff.length());
}

void Logger::info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    write("info", color_info, fmt, ap);
    va_end(ap);
}

void Logger::debug(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    write("debug", color_debug, fmt, ap);
    va_end(ap);
}

void Logger::warning(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    write("warn", color_warning, fmt, ap);
    va_end(ap);
}
void Logger::error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    write("error", color_error, fmt, ap);
    va_end(ap);
}

Logger logger("net");

void ElapsedTime::start() {
    struct timezone tz;
    gettimeofday(&t0, &tz);
    cpu_t0 = clock();
}

void ElapsedTime::stop(bool show_info) {
    struct timeval t1;
    struct timezone tz;
    gettimeofday(&t1, &tz);
    cpu_elapsed_sec = (float)clock() / CLOCKS_PER_SEC -
                        (float)cpu_t0 / CLOCKS_PER_SEC;
    elapsed_sec = (t1.tv_sec + t1.tv_usec * 1e-6) -
                    (t0.tv_sec + t0.tv_usec * 1e-6);
    if (show_info)
        SALTICIDAE_LOG_INFO("elapsed: %.3f (wall) %.3f (cpu)",
                    elapsed_sec, cpu_elapsed_sec);
}

Config::Opt::Opt(const std::string &optname, const std::string &optdoc,
                const optval_t &optval, Action action,
                char short_opt,
                int idx):
    optname(optname), optdoc(optdoc),
    optval(optval), action(action),
    short_opt(short_opt) {
    opt.name = this->optname.c_str();
    opt.has_arg = action == SWITCH_ON ? no_argument : required_argument;
    opt.flag = nullptr;
    opt.val = 0x100 + idx;
}

void Config::add_opt(const std::string &optname, const optval_t &optval, Action action,
                    char short_opt,
                    const std::string &optdoc) {
    if (conf.count(optname))
        throw SalticidaeError(SALTI_ERROR_OPTNAME_ALREADY_EXISTS);
    opts.push_back(new Opt(optname, optdoc,
                            optval, action, short_opt,
                            opts.size()));
    auto opt = opts.back().get();
    conf.insert(std::make_pair(optname, opt));
    if (short_opt != -1)
        conf.insert(std::make_pair(std::string(1, short_opt), opt));
}

void Config::update(Opt &p, const char *optval) {
    switch (p.action)
    {
        case SWITCH_ON: p.optval->switch_on(); break;
        case SET_VAL: p.optval->set_val(optval); break;
        case APPEND: p.optval->append(optval); break;
        default:
            throw SalticidaeError(SALTI_ERROR_OPT_UNKNOWN_ACTION);
    }
}

void Config::update(const std::string &optname, const char *optval) {
    assert(conf.count(optname));
    update(*(conf.find(optname)->second), optval);
}

bool Config::load(const std::string &fname) {
    static const size_t BUFF_SIZE = 65536;
    FILE *conf_f = fopen(fname.c_str(), "r");
    char buff[BUFF_SIZE];
    /* load configuration from file */
    if (conf_f)
    {
        while (fgets(buff, BUFF_SIZE, conf_f))
        {
            if (strlen(buff) == BUFF_SIZE - 1)
            {
                fclose(conf_f);
                throw SalticidaeError(SALTI_ERROR_CONFIG_LINE_TOO_LONG);
            }
            std::string line(buff);
            size_t pos = line.find("=");
            if (pos == std::string::npos)
                continue;
            std::string optname = trim(line.substr(0, pos));
            std::string optval = trim(line.substr(pos + 1));
            if (!conf.count(optname))
            {
                SALTICIDAE_LOG_WARN("ignoring option name in conf file: %s",
                            optname.c_str());
                continue;
            }
            update(optname, optval.c_str());
        }
        fclose(conf_f);
        return true;
    }
    else
        return false;
}

size_t Config::parse(int argc, char **argv) {
    if (load(conf_fname))
        SALTICIDAE_LOG_INFO("loaded configuration from %s", conf_fname.c_str());

    size_t nopts = opts.size();
    auto longopts = BoxObj<struct option[]>(new struct option[nopts + 1]);
    int ind;
    std::string shortopts;
    for (size_t i = 0; i < nopts; i++)
    {
        const auto &opt = opts[i];
        longopts[i] = opt->opt;
        if (opt->short_opt != -1)
        {
            shortopts += opt->short_opt;
            if (longopts[i].has_arg == required_argument)
                shortopts += ":";
        }
    }
    longopts[nopts] = {0, 0, 0, 0};
    for (;;)
    {
        int id = getopt_long(argc, argv, shortopts.c_str(), longopts.get(), &ind);
        if (id == -1)
            break;
        if (id == '?')
            throw SalticidaeError(SALTI_ERROR_OPT_INVALID);
        if (id >= 0x100)
            update(*(opts[id - 0x100]), optarg);
        else
            update(std::string(1, (char)id), optarg);
    }
    return optind;
}

void Config::print_help(FILE *output) {
    int width = 0;
    for (const auto &opt: opts)
        width = std::max(width, (int)opt->optname.length());
    for (const auto &opt: opts)
    {
        fprintf(output, "  --%s%*c", opt->optname.c_str(),
                width - (int)opt->optname.length() + 4, ' ');
        if (opt->short_opt != -1)
            fprintf(output, "-%c\t", opt->short_opt);
        else
            fprintf(output, "\t\t");
        fprintf(output, "%s\n", opt->optdoc.c_str());
    }
}

std::vector<std::string> split(const std::string &s, const std::string &delim) {
    std::vector<std::string> res;
    size_t lastpos = 0;
    for (;;)
    {
        size_t pos = s.find(delim, lastpos);
        if (pos == std::string::npos)
            break;
        res.push_back(s.substr(lastpos, pos - lastpos));
        lastpos = pos + delim.length();
    }
    res.push_back(s.substr(lastpos, s.length() - lastpos));
    return res;
}

std::string trim(const std::string &s, const std::string &space) {
    const auto new_begin = s.find_first_not_of(space);
    if (new_begin == std::string::npos)
        return "";
    const auto new_end = s.find_last_not_of(space);
    return s.substr(new_begin, new_end - new_begin + 1);
}

std::vector<std::string> trim_all(const std::vector<std::string> &ss) {
    std::vector<std::string> res;
    for (const auto &s: ss)
        res.push_back(trim(s));
    return res;
}

}
