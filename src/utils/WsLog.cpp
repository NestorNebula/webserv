/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WsLog.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 11:56:36 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/05 17:30:20 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WsLog.hpp"

log_lvl     WsLog::lvl = LVL_NONE;
log_tgt     WsLog::tgt = TGT_NONE;
std::string WsLog::col;

static const std::string tgt_str[] =
{
    "",
    "epoll : ",
    "epc   : ",
    "conn  : ",
    "cgi   : ",
    "env   : ",
    "serv  : ",
    "main  : ",
    "head  : ",
    "body  : ",
    "rsrc  : ",
    "fcgi  : "
};

// so .. log .. takes more time 
// cgi .. finishes in "background" sooner (?)

// Lots of writes to stderr can confuse socket communication by causing I/O blocking, buffer saturation, and timing disruptions in the application event loop. When a program spams error logs, it starves network tasks of CPU time and resources.

// While the CPU waits for stderr to clear, it cannot read from or write to the network socket


// Why Logging Interferes with SocketsBlocking I/O: Writing to stderr often blocks execution if the destination stream (like a terminal or a slow log file) cannot process data instantly. While the CPU waits for stderr to clear, it cannot read from or write to the network socket.

// Buffer Backpressure: If stderr fills up operating system pipes, the process pauses. This delay prevents the app from clearing incoming socket buffers, triggering remote timeouts.

// Event Loop Starvation: In single-threaded event loops (like Node.js or Python asyncio), synchronous or heavy logging operations monopolize the thread. The application fails to poll socket descriptors, delaying packet reads and handshakes.

static const std::string &tgt_prefix(log_tgt tgt)
{
    if (tgt & TGT_EPOLL)
        return (tgt_str[1]);
    if (tgt & TGT_EPC)
        return (tgt_str[2]);
    if (tgt & TGT_CONN)
        return (tgt_str[3]);
    if (tgt & TGT_CGI)
        return (tgt_str[4]);
    if (tgt & TGT_CGI_ENV)
        return (tgt_str[5]);
    if (tgt & TGT_SERV)
        return (tgt_str[6]);
    if (tgt & TGT_MAIN)
        return (tgt_str[7]);
    if (tgt & (TGT_HEAD | TGT_CGI_HEAD))
        return (tgt_str[8]);
    if (tgt & TGT_BODY)
        return (tgt_str[9]);
    if (tgt & (TGT_RSRC | TGT_RSRC_INFO | TGT_RSRC_WAIT | TGT_RSRC_STAT))
        return (tgt_str[10]);
    if (tgt & TGT_FCGI)
        return (tgt_str[11]);

    return (tgt_str[0]);
}

bool    WsLog::nolog(log_lvl msg_lvl, log_tgt msg_tgt)
{
    bool skip = true;

    switch (msg_lvl)
    {
    case LVL_ERR:
    case LVL_TMP:
        skip = false;
        break;
    default:
        if ((msg_lvl & WsLog::lvl) && (msg_tgt & WsLog::tgt))
            skip = false;
        break;
    }
    if (skip)
        WsLog::col.clear();
    return (skip);
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << msg;
    WsLog::op(stream);
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, ssize_t n)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << msg << "[" << n << "]";
    WsLog::op(stream);
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, ssize_t i, ssize_t j)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << msg << "[" << i << " / " << j << "]";
    WsLog::op(stream);
}


void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, std::string str)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << msg << str;
    WsLog::op(stream);
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, ssize_t n)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << n;
    WsLog::op(stream);
}

int	WsLog::_errno(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg)
{
    (void) msg_lvl;
    
    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << msg << "\n";
    stream << "error : " << strerror(errno);
    WsLog::op(stream);

    return (-1);
}
void WsLog::op(std::stringstream & stream)
{
    std::cerr << WsLog::col << stream.str() << "\n" << std::string("\e[0m");
    WsLog::col.clear();
}

void WsLog::color(int c)
{
    switch(c)
    {
    case WSL_RED:
        WsLog::col = std::string("\e[1;31m");
        break;
    case WSL_GREEN:
        WsLog::col = std::string("\e[1;32m");
        break;
    case WSL_YELLOW:
        WsLog::col = std::string("\e[1;33m");
        break;
    case 0:
    default:
        WsLog::col = std::string("SUCK");
    }
}
#if 0

# Not source who sources this ...

# Reset
Color_Off='\e[0m'       # Text Reset

# Regular Colors
Black='\e[0;30m'        # Black
Red='\e[0;31m'          # Red
Green='\e[0;32m'        # Green
Yellow='\e[0;33m'       # Yellow
Blue='\e[0;34m'         # Blue
Purple='\e[0;35m'       # Purple
Cyan='\e[0;36m'         # Cyan
White='\e[0;37m'        # White

# Bold
BBlack='\e[1;30m'       # Black
BRed='\e[1;31m'         # Red
BGreen='\e[1;32m'       # Green
BYellow='\e[1;33m'      # Yellow
BBlue='\e[1;34m'        # Blue
BPurple='\e[1;35m'      # Purple
BCyan='\e[1;36m'        # Cyan
BWhite='\e[1;37m'       # White

# Underline
UBlack='\e[4;30m'       # Black
URed='\e[4;31m'         # Red
UGreen='\e[4;32m'       # Green
UYellow='\e[4;33m'      # Yellow
UBlue='\e[4;34m'        # Blue
UPurple='\e[4;35m'      # Purple
UCyan='\e[4;36m'        # Cyan
UWhite='\e[4;37m'       # White

# Background
On_Black='\e[40m'       # Black
On_Red='\e[41m'         # Red
On_Green='\e[42m'       # Green
On_Yellow='\e[43m'      # Yellow
On_Blue='\e[44m'        # Blue
On_Purple='\e[45m'      # Purple
On_Cyan='\e[46m'        # Cyan
On_White='\e[47m'       # White

# High Intensity
IBlack='\e[0;90m'       # Black
IRed='\e[0;91m'         # Red
IGreen='\e[0;92m'       # Green
IYellow='\e[0;93m'      # Yellow
IBlue='\e[0;94m'        # Blue
IPurple='\e[0;95m'      # Purple
ICyan='\e[0;96m'        # Cyan
IWhite='\e[0;97m'       # White

# Bold High Intensity
BIBlack='\e[1;90m'      # Black
BIRed='\e[1;91m'        # Red
BIGreen='\e[1;92m'      # Green
BIYellow='\e[1;93m'     # Yellow
BIBlue='\e[1;94m'       # Blue
BIPurple='\e[1;95m'     # Purple
BICyan='\e[1;96m'       # Cyan
BIWhite='\e[1;97m'      # White

# High Intensity backgrounds
On_IBlack='\e[0;100m'   # Black
On_IRed='\e[0;101m'     # Red
On_IGreen='\e[0;102m'   # Green
On_IYellow='\e[0;103m'  # Yellow
On_IBlue='\e[0;104m'    # Blue
On_IPurple='\e[0;105m'  # Purple
On_ICyan='\e[0;106m'    # Cyan
On_IWhite='\e[0;107m'   # White


txtblk='\e[0;30m' # Black - Regular
txtred='\e[0;31m' # Red
txtgrn='\e[0;32m' # Green
txtylw='\e[0;33m' # Yellow
txtblu='\e[0;34m' # Blue
txtpur='\e[0;35m' # Purple
txtcyn='\e[0;36m' # Cyan
txtwht='\e[0;37m' # White
bldblk='\e[1;30m' # Black - Bold
bldred='\e[1;31m' # Red
bldgrn='\e[1;32m' # Green
bldylw='\e[1;33m' # Yellow
bldblu='\e[1;34m' # Blue
bldpur='\e[1;35m' # Purple
bldcyn='\e[1;36m' # Cyan
bldwht='\e[1;37m' # White
unkblk='\e[4;30m' # Black - Underline
undred='\e[4;31m' # Red
undgrn='\e[4;32m' # Green
undylw='\e[4;33m' # Yellow
undblu='\e[4;34m' # Blue
undpur='\e[4;35m' # Purple
undcyn='\e[4;36m' # Cyan
undwht='\e[4;37m' # White
bakblk='\e[40m'   # Black - Background
bakred='\e[41m'   # Red
bakgrn='\e[42m'   # Green
bakylw='\e[43m'   # Yellow
bakblu='\e[44m'   # Blue
bakpur='\e[45m'   # Purple
bakcyn='\e[46m'   # Cyan
bakwht='\e[47m'   # White
txtrst='\e[0m'    # Text Reset

#endif

void    WsLog::kd(void)
{
    WsLog::lvl = LVL_NONE
        | LVL_ERR
        | LVL_WARN
        | LVL_INFO
        | LVL_DBG
    ;
    WsLog::tgt = TGT_NONE
        // | TGT_EPOLL 
        | TGT_EPOLL_EVT
        | TGT_EPOLL_CTL
        
        // | TGT_EPC
        // | TGT_EPC_RECV
        // | TGT_EPC_SEND
        
        
        | TGT_CONN
        // | TGT_CONN_RECV
        // | TGT_CONN_SEND
        // | TGT_CONN_DATA

        | TGT_CGI
        // | TGT_CGI_RECV
        // | TGT_CGI_SEND
        // | TGT_CGI_DATA
        | TGT_CGI_HEAD

        | TGT_SERV
        // | TGT_MAIN

        // | TGT_HEAD
        // | TGT_BODY
        | TGT_RSRC
        | TGT_RSRC_INFO
        | TGT_RSRC_STAT
        // | TGT_RSRC_WAIT
    ;
    
    // WsLog::tgt = TGT_NONE;
    WsLog::tgt = TGT_EPOLL_EVT | TGT_CONN_SEND | TGT_RSRC_STAT | TGT_RSRC_WAIT;

    // WsLog::lvl = LVL_INFO;
    // WsLog::tgt = TGT_ALL;
}