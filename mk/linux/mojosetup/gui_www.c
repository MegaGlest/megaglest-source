/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 *
       Copyright (c) 2006-2010 Ryan C. Gordon and others.

   This software is provided 'as-is', without any express or implied warranty.
   In no event will the authors be held liable for any damages arising from
   the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software in a
   product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source distribution.

       Ryan C. Gordon <icculus@icculus.org>
 *
 */

#if !SUPPORT_GUI_WWW
#error Something is wrong in the build system.
#endif

#define BUILDING_EXTERNAL_PLUGIN 1
#include "gui.h"

MOJOGUI_PLUGIN(www)

#if !GUI_STATIC_LINK_WWW
CREATE_MOJOGUI_ENTRY_POINT(www)
#endif

#include <stdarg.h>

#define FREE_AND_NULL(x) { free(x); x = NULL; }

// tapdance between things WinSock and BSD Sockets define differently...
#if PLATFORM_WINDOWS
    #include <winsock.h>

    typedef int socklen_t;

    #define setprotoent(x) assert(x == 0)
    #define sockErrno() WSAGetLastError()
    #define wouldBlockError(err) (err == WSAEWOULDBLOCK)
    #define intrError(err) (err == WSAEINTR)

    static inline void setBlocking(SOCKET s, boolean blocking)
    {
        u_long val = (blocking) ? 0 : 1;
        ioctlsocket(s, FIONBIO, &val);
    } // setBlocking

    static const char *sockStrErrVal(int val)
    {
        STUBBED("Windows strerror");
        return "sockStrErrVal() is unimplemented.";
    } // sockStrErrVal


    static boolean initSocketSupport(void)
    {
        WSADATA data;
        int rc = WSAStartup(MAKEWORD(1, 1), &data);
        if (rc != 0)
        {
            logError("www: WSAStartup() failed: %0", sockStrErrVal(rc));
            return false;
        } // if

        logInfo("www: WinSock initialized (want %0.%1, got %2.%3).",
                        numstr((int) (LOBYTE(data.wVersion))),
                        numstr((int) (HIBYTE(data.wVersion))),
                        numstr((int) (LOBYTE(data.wHighVersion))),
                        numstr((int) (HIBYTE(data.wHighVersion))));
        logInfo("www: WinSock description: %0", data.szDescription);
        logInfo("www: WinSock system status: %0", data.szSystemStatus);
        logInfo("www: WinSock max sockets: %0", numstr((int) data.iMaxSockets));

        return true;
    } // initSocketSupport

    #define deinitSocketSupport() WSACleanup()

#else
    #include <unistd.h>
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <signal.h>
    #include <netdb.h>
    #include <fcntl.h>

    typedef int SOCKET;

    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET (-1)
    #define closesocket(x) close(x)
    #define sockErrno() (errno)
    #define sockStrErrVal(val) strerror(val)
    #define intrError(err) (err == EINTR)
    #define initSocketSupport() (true)
    #define deinitSocketSupport()

    static inline boolean wouldBlockError(int err)
    {
        return ((err == EWOULDBLOCK) || (err == EAGAIN));
    } // wouldBlockError

    static void setBlocking(SOCKET s, boolean blocking)
    {
        int flags = fcntl(s, F_GETFL, 0);
        if (blocking)
            flags &= ~O_NONBLOCK;
        else
            flags |= O_NONBLOCK;
        fcntl(s, F_SETFL, flags);
    } // setBlocking

#endif

#define sockStrError() (sockStrErrVal(sockErrno()))


typedef struct _S_WebRequest
{
    char *key;
    char *value;
    struct _S_WebRequest *next;
} WebRequest;


static char *output = NULL;
static char *lastProgressType = NULL;
static char *lastComponent = NULL;
static char *baseUrl = NULL;
static WebRequest *webRequest = NULL;
static uint32 percentTicks = 0;
static SOCKET listenSocket = INVALID_SOCKET;
static SOCKET clientSocket = INVALID_SOCKET;


static uint8 MojoGui_www_priority(boolean istty)
{
    return MOJOGUI_PRIORITY_TRY_LAST;
} // MojoGui_www_priority


static void freeWebRequest(void)
{
    while (webRequest)
    {
        WebRequest *next = webRequest->next;
        free(webRequest->key);
        free(webRequest->value);
        free(webRequest);
        webRequest = next;
    } // while
} // freeWebRequest


static void addWebRequest(const char *key, const char *val)
{
    if ((key != NULL) && (*key != '\0'))
    {
        WebRequest *req = (WebRequest *) xmalloc(sizeof (WebRequest));
        req->key = xstrdup(key);
        req->value = xstrdup(val);
        req->next = webRequest;
        webRequest = req;
        logDebug("www: request element '%0' = '%1'", key, val);
    } // if
} // addWebRequest


static int hexVal(char ch)
{
    if ((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a') + 10;
    else if ((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A') + 10;
    else if ((ch >= '0') && (ch <= '9'))
        return (ch - '0');
    return -1;
} // hexVal


static void unescapeUri(char *uri)
{
    char *ptr = uri;
    while ((ptr = strchr(ptr, '%')) != NULL)
    {
        int a, b;
        if ((a = hexVal(ptr[1])) != -1)
        {
            if ((b = hexVal(ptr[2])) != -1)
            {
                *(ptr++) = (char) ((a * 16) + b);
                memmove(ptr, ptr+2, strlen(ptr+1));
            } // if
            else
            {
                *(ptr++) = '?';
                memmove(ptr, ptr+1, strlen(ptr));
            } // else
        } // if
        else
        {
            *(ptr++) = '?';
        } // else
    } // while
} // unescapeUri


static int strAdd(char **ptr, size_t *len, size_t *alloc, const char *fmt, ...)
{
    size_t bw = 0;
    size_t avail = *alloc - *len;
    va_list ap;
    va_start(ap, fmt);
    bw = vsnprintf(*ptr + *len, avail, fmt, ap);
    va_end(ap);

    if (bw >= avail)
    {
        const size_t add = (*alloc + (bw + 1));  // double plus the new len.
        *alloc += add;
        avail += add;
        *ptr = xrealloc(*ptr, *alloc);
        va_start(ap, fmt);
        bw = vsnprintf(*ptr + *len, avail, fmt, ap);
        va_end(ap);
    } // if

    *len += bw;
    return bw;
} // strAdd


static char *htmlescape(const char *str)
{
    size_t len = 0, alloc = 0;
    char *retval = NULL;
    char ch;

    while ((ch = *(str++)) != '\0')
    {
        switch (ch)
        {
            case '&': strAdd(&retval, &len, &alloc, "&amp;"); break;
            case '<': strAdd(&retval, &len, &alloc, "&lt;"); break;
            case '>': strAdd(&retval, &len, &alloc, "&gt;"); break;
            case '"': strAdd(&retval, &len, &alloc, "&quot;"); break;
            case '\'': strAdd(&retval, &len, &alloc, "&#39;"); break;
            default: strAdd(&retval, &len, &alloc, "%c", ch); break;
        } // switch
    } // while

    return retval;
} // htmlescape


static const char *standardResponseHeaders =
    "Content-Type: text/html; charset=utf-8\n"
    "Accept-Ranges: none\n"
    "Cache-Control: no-cache\n"
    "Connection: close\n\n";


static void setHtmlString(char **str, int responseCode,
                          const char *responseString,
                          const char *title, const char *html)
{
    size_t len = 0, alloc = 0;
    FREE_AND_NULL(*str);
    strAdd(str, &len, &alloc,
              "HTTP/1.1 %d %s\n"   // responseCode, responseString
              "%s"  // standardResponseHeaders
              "<html>"
               "<head>"
                "<title>%s</title>"  // title
               "</head>"
               "<body>%s</body>"  // html
              "</html>\n",
           responseCode, responseString,
           standardResponseHeaders,
           title, html);
} // setHtmlString


static void setHtml(const char *title, const char *html)
{
    setHtmlString(&output, 200, "OK", title, html);
} // setHtml


static void sendStringAndDrop(SOCKET *_s, const char *str)
{
    SOCKET s = *_s;
    int outlen = 0;
    if (str == NULL)
        str = "";
    else
        outlen = strlen(str);

    setBlocking(s, true);

    while (outlen > 0)
    {
        int rc = send(s, str, outlen, 0);
        if (rc != SOCKET_ERROR)
        {
            str += rc;
            outlen -= rc;
        } // if
        else
        {
            const int err = sockErrno();
            if (!intrError(err))
            {
                logError("www: send() failed: %0", sockStrErrVal(err));
                break;
            } // if
        } // else
    } // while

    closesocket(s);
    *_s = INVALID_SOCKET;
} // sendStringAndDrop


static void respond404(SOCKET *s)
{
    char *text = htmlescape(_("Not Found"));
    char *str = NULL;
    size_t len = 0, alloc = 0;
    char *html = NULL;
    strAdd(&html, &len, &alloc, "<center><h1>%s</h1></center>", text);
    setHtmlString(&str, 404, text, text, html);
    free(html);
    free(text);
    sendStringAndDrop(s, str);
    free(str);
} // respond404


static boolean parseGet(char *get)
{
    char *uri = NULL;
    char *ver = NULL;

    uri = strchr(get, ' ');
    if (uri == NULL) return false;
    *(uri++) = '\0';

    ver = strchr(uri, ' ');
    if (ver == NULL) return false;
    *(ver++) = '\0';

    if (strcmp(get, "GET") != 0) return false;
    if (uri[0] != '/') return false;
    uri++;  // skip dirsep.

    // !!! FIXME: we may want to feed stock files (<img> tags, etc)
    // !!! FIXME:  at some point in the future.
    if ((uri[0] != '?') && (uri[0] != '\0')) return false;
    if (strncmp(ver, "HTTP/", 5) != 0) return false;

    if (*uri == '?')
        uri++;  // skip initial argsep.

    do
    {
        char *next = strchr(uri, '&');
        char *val = NULL;
        if (next != NULL)
            *(next++) = '\0';

        val = strchr(uri, '=');
        if (val == NULL)
            val = "";
        else
            *(val++) = '\0';

        unescapeUri(uri);
        unescapeUri(val);
        addWebRequest(uri, val);

        uri = next;
    } while (uri != NULL);

    return true;
} // parseGet


static boolean parseRequest(char *reqstr)
{
    do
    {
        char *next = strchr(reqstr, '\n');
        char *val = NULL;
        if (next != NULL)
            *(next++) = '\0';

        val = strchr(reqstr, ':');
        if (val == NULL)
            val = "";
        else
        {
            *(val++) = '\0';
            while (*val == ' ')
                val++;
        } // else

        if (*reqstr != '\0')
        {
            size_t len = 0, alloc = 0;
            char *buf = NULL;
            strAdd(&buf, &len, &alloc, "HTTP-%s", reqstr);
            addWebRequest(buf, val);
            free(buf);
        } // if

        reqstr = next;
    } while (reqstr != NULL);

    return true;
} // parseRequest



static WebRequest *servePage(boolean blocking)
{
    int newline = 0;
    char ch = 0;
    struct sockaddr_in addr;
    socklen_t addrlen = 0;
    int s = 0;
    char *reqstr = NULL;
    size_t len = 0, alloc = 0;
    int err = 0;

    freeWebRequest();

    if (listenSocket == INVALID_SOCKET)
        return NULL;

    if (clientSocket != INVALID_SOCKET)  // response to feed to client.
        sendStringAndDrop(&clientSocket, output);

    if (blocking)
        setBlocking(listenSocket, true);

    do
    {
        s = accept(listenSocket, (struct sockaddr *) &addr, &addrlen);
        err = sockErrno();
    } while ( (s == INVALID_SOCKET) && (intrError(err)) );

    if (blocking)
        setBlocking(listenSocket, false);  // reset what we toggled up there.

    if (s == INVALID_SOCKET)
    {
        if (wouldBlockError(err))
            assert(!blocking);
        else
        {
            logError("www: accept() failed: %0", sockStrErrVal(err));
            closesocket(listenSocket);  // make all future i/o fail too.
            listenSocket = INVALID_SOCKET;
        } // else
        return NULL;
    } // if

    setBlocking(s, true);

    // Doing this one char at a time isn't efficient, but it's easy.

    while (1)
    {
        if (recv(s, &ch, 1, 0) == SOCKET_ERROR)
        {
            const int err = sockErrno();
            if (!intrError(err))  // just try again on interrupt.
            {
                logError("www: recv() failed: %0", sockStrErrVal(err));
                FREE_AND_NULL(reqstr);
                closesocket(s);
                s = INVALID_SOCKET;
                break;
            } // if
        } // if

        else if (ch == '\n')  // newline
        {
            if (++newline == 2)
                break;  // end of request.
            strAdd(&reqstr, &len, &alloc, "\n");
        } // if

        else if (ch != '\r')
        {
            newline = 0;
            strAdd(&reqstr, &len, &alloc, "%c", ch);
        } // else if
    } // while

    if (reqstr != NULL)
    {
        char *get = NULL;
        char *ptr = strchr(reqstr, '\n');
        if (ptr != NULL)
        {
            *ptr = '\0';
            ptr++;
        } // if

        // reqstr is the GET (or whatever) request, ptr is the rest.
        get = xstrdup(reqstr);
        if (ptr == NULL)
        {
            *ptr = '\0';
            len = 0;
        } // if
        else
        {
            len = strlen(ptr);
            memmove(reqstr, ptr, len+1);
        } // else

        logDebug("www: request '%0'", get);

        // okay, now (get) and (reqptr) are separate strings.
        // These parse*() functions update (webRequest).
        if ( (parseGet(get)) && (parseRequest(reqstr)) )
            logDebug("www: accepted request");
        else
        {
            logError("www: rejected bogus request");
            freeWebRequest();
            respond404(&s);
        } // else

        free(reqstr);
        free(get);
    } // if

    clientSocket = s;
    return webRequest;
} // servePage


static SOCKET create_listen_socket(short portnum)
{
    SOCKET s = INVALID_SOCKET;
    int protocol = 0;  // pray this is right.
    struct protoent *prot;

    setprotoent(0);
    prot = getprotobyname("tcp");
    if (prot != NULL)
        protocol = prot->p_proto;

    s = socket(PF_INET, SOCK_STREAM, protocol);
    if (s == INVALID_SOCKET)
        logInfo("www: socket() failed ('%0')", sockStrError());
    else
    {
        boolean success = false;
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(portnum);
        addr.sin_addr.s_addr = INADDR_ANY;  // !!! FIXME: bind to localhost.

        // So we can bind this socket over and over in debug runs...
        #if ((!defined _NDEBUG) && (!defined NDEBUG))
        {
            int on = 1;
            setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof (on));
        }
        #endif

        if (bind(s, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
            logError("www: bind() failed ('%0')", sockStrError());
        else if (listen(s, 5) == SOCKET_ERROR)
            logError("www: listen() failed ('%0')", sockStrError());
        else
        {
            logInfo("www: socket created on port %0",
                           numstr(portnum));
            success = true;
        } // else

        if (!success)
        {
            closesocket(s);
            s = INVALID_SOCKET;
        } // if
    } // if

    return s;
} // create_listen_socket


static boolean MojoGui_www_init(void)
{
    size_t len = 0, alloc = 0;
    short portnum = 7341;  // !!! FIXME: try some random ports.
    percentTicks = 0;

    if (!initSocketSupport())
    {
        logInfo("www: socket subsystem init failed, use another UI.");
        return false;
    } // if
    
    listenSocket = create_listen_socket(portnum);
    if (listenSocket < 0)
    {
        logInfo("www: no listen socket, use another UI.");
        return false;
    } // if

    setBlocking(listenSocket, false);

    strAdd(&baseUrl, &len, &alloc, "http://localhost:%d/", (int) portnum);
    return true;
} // MojoGui_www_init


static void MojoGui_www_deinit(void)
{
    // Catch any waiting browser connections...and tell them to buzz off!  :)
    char *donetitle = htmlescape(_("Shutting down..."));
    char *donetext = htmlescape(_("You can close this browser now."));
    size_t len = 0, alloc = 0;
    char *html = NULL;

    strAdd(&html, &len, &alloc, "<hr><center>%s</center><hr>", donetext);
    setHtml(donetitle, html);
    free(html);
    free(donetitle);
    free(donetext);
    while (servePage(false) != NULL) { /* no-op. */ }

    freeWebRequest();
    FREE_AND_NULL(output);
    FREE_AND_NULL(lastProgressType);
    FREE_AND_NULL(lastComponent);
    FREE_AND_NULL(baseUrl);

    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    } // if

    if (listenSocket != INVALID_SOCKET)
    {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    } // if

    deinitSocketSupport();
} // MojoGui_www_deinit


static int doPromptPage(const char *title, const char *text, boolean centertxt,
                        const char *pagename,
                        const char **buttons, const char **locButtons,
                        int bcount)
{
    char *htmltitle = htmlescape(title);
    boolean sawPage = false;
    int answer = -1;
    int i = 0;
    char *html = NULL;
    size_t len = 0, alloc = 0;
    const char *align = ((centertxt) ? " align='center'" : "");

    strAdd(&html, &len, &alloc,
        "<center>"
        "<form name='form_%s' method='get'>"  // pagename
         "<input type='hidden' name='page' value='%s'>"  // pagename
         "<table>"
          "<tr><td%s>%s</td></tr>"   // align, text
          "<tr>"
           "<td align='center'>", pagename, pagename, align, text);

    for (i = 0; i < bcount; i++)
    {
        const char *button = buttons[i];
        const char *loc = locButtons[i];
        strAdd(&html, &len, &alloc,
            "<input type='submit' name='%s' value='%s'>", button, loc);
    } // for

    strAdd(&html, &len, &alloc,
           "</td>"
          "</tr>"
         "</table>"
        "</form>"
        "</center>");

    setHtml(htmltitle, html);
    free(htmltitle);
    free(html);

    while ((!sawPage) || (answer == -1))
    {
        WebRequest *req = servePage(true);
        sawPage = false;
        answer = -1;
        while (req != NULL)
        {
            const char *k = req->key;
            const char *v = req->value;
            if ( (strcmp(k, "page") == 0) && (strcmp(v, pagename) == 0) )
                sawPage = true;
            else
            {
                for (i = 0; i < bcount; i++)
                {
                    if (strcmp(k, buttons[i]) == 0)
                    {
                        answer = i;
                        break;
                    } // if
                } // for
            } // else

            req = req->next;
        } // while
    } // while

    return answer;
} // doPromptPage


static void MojoGui_www_msgbox(const char *title, const char *text)
{
    const char *buttons[] = { "ok" };
    const char *locButtons[] = { htmlescape(_("OK")) };
    char *htmltext = htmlescape(text);
    doPromptPage(title, htmltext, true, "msgbox", buttons, locButtons, 1);
    free(htmltext);
    free((void *) locButtons[0]);
} // MojoGui_www_msgbox


static boolean MojoGui_www_promptyn(const char *title, const char *text,
                                    boolean defval)
{
    // !!! FIXME:
    // We currently ignore defval

    int i, rc;
    char *htmltext = htmlescape(text);
    const char *buttons[] = { "no", "yes" };
    const char *locButtons[] = { htmlescape(_("No")), htmlescape(_("Yes")) };

    assert(STATICARRAYLEN(buttons) == STATICARRAYLEN(locButtons));

    rc = doPromptPage(title, htmltext, true, "promptyn", buttons, locButtons,
                      STATICARRAYLEN(buttons));
    free(htmltext);

    for (i = 0; i < STATICARRAYLEN(locButtons); i++)
        free((void *) locButtons[i]);

    return (rc == 1);
} // MojoGui_www_promptyn


static MojoGuiYNAN MojoGui_www_promptynan(const char *title, const char *text,
                                          boolean defval)
{
    // !!! FIXME:
    // We currently ignore defval

    int i, rc;
    char *htmltext = htmlescape(text);
    const char *buttons[] = { "no", "yes", "always", "never" };
    const char *locButtons[] = {
        htmlescape(_("No")),
        htmlescape(_("Yes")),
        htmlescape(_("Always")),
        htmlescape(_("Never")),
    };
    assert(STATICARRAYLEN(buttons) == STATICARRAYLEN(locButtons));

    rc = doPromptPage(title, htmltext, true, "promptynan", buttons, locButtons,
                      STATICARRAYLEN(buttons));

    free(htmltext);
    for (i = 0; i < STATICARRAYLEN(locButtons); i++)
        free((void *) locButtons[i]);

    return (MojoGuiYNAN) rc;
} // MojoGui_www_promptynan


static boolean MojoGui_www_start(const char *title,
                                 const MojoGuiSplash *splash)
{
    return true;
} // MojoGui_www_start


static void MojoGui_www_stop(void)
{
    // no-op.
} // MojoGui_www_stop


static int MojoGui_www_readme(const char *name, const uint8 *data,
                              size_t datalen, boolean can_back,
                              boolean can_fwd)
{
    char *text = NULL;
    size_t len = 0, alloc = 0;
    char *htmldata = htmlescape((const char *) data);
    int i, rc;
    int cancelbutton = -1;
    int backbutton = -1;
    int fwdbutton = -1;
    int bcount = 0;
    const char *buttons[4] = { NULL, NULL, NULL, NULL };
    const char *locButtons[4] = { NULL, NULL, NULL, NULL };
    assert(STATICARRAYLEN(buttons) == STATICARRAYLEN(locButtons));

    cancelbutton = bcount++;
    buttons[cancelbutton] = "cancel";
    locButtons[cancelbutton] = xstrdup(_("Cancel"));

    if (can_back)
    {
        backbutton = bcount++;
        buttons[backbutton] = "back";
        locButtons[backbutton] = xstrdup(_("Back"));
    } // if

    if (can_fwd)
    {
        fwdbutton = bcount++;
        buttons[fwdbutton] = "next";
        locButtons[fwdbutton] = xstrdup(_("Next"));
    } // if

    strAdd(&text, &len, &alloc, "<pre>\n%s\n</pre>", htmldata);
    free(htmldata);

    rc = doPromptPage(name, text, false, "readme", buttons, locButtons, bcount);

    free(text);
    for (i = 0; i < STATICARRAYLEN(locButtons); i++)
        free((void *) locButtons[i]);

    if (rc == backbutton)
        return -1;
    else if (rc == cancelbutton)
        return 0;
    return 1;
} // MojoGui_www_readme


static int MojoGui_www_options(MojoGuiSetupOptions *opts,
                               boolean can_back, boolean can_fwd)
{
    // !!! FIXME: write me.
    STUBBED("www options");
    return 1;
} // MojoGui_www_options


static char *MojoGui_www_destination(const char **recommends, int recnum,
                                     int *command, boolean can_back,
                                     boolean can_fwd)
{
    char *retval = NULL;
    char *title = xstrdup(_("Destination"));
    char *html = NULL;
    size_t len = 0, alloc = 0;
    boolean checked = true;
    int cancelbutton = -1;
    int backbutton = -1;
    int fwdbutton = -1;
    int bcount = 0;
    int rc = 0;
    int i = 0;
    const char *buttons[4] = { NULL, NULL, NULL, NULL };
    const char *locButtons[4] = { NULL, NULL, NULL, NULL };
    assert(STATICARRAYLEN(buttons) == STATICARRAYLEN(locButtons));

    cancelbutton = bcount++;
    buttons[cancelbutton] = "cancel";
    locButtons[cancelbutton] = xstrdup(_("Cancel"));

    if (can_back)
    {
        backbutton = bcount++;
        buttons[backbutton] = "back";
        locButtons[backbutton] = xstrdup(_("Back"));
    } // if

    if (can_fwd)
    {
        fwdbutton = bcount++;
        buttons[fwdbutton] = "next";
        locButtons[fwdbutton] = xstrdup(_("Next"));
    } // if

    strAdd(&html, &len, &alloc,
        "<form name='form_destination' method='get'>"
          "<table>");

    for (i = 0; i < recnum; i++)
    {
        strAdd(&html, &len, &alloc,
           "<tr>"
            "<td>"
             "<input type='radio' name='dest' %s value='%s'>%s"
            "</td>"
           "</tr>",
            ((checked) ? "checked='true'" : ""), recommends[i], recommends[i]);
        checked = false;
    } // for

    strAdd(&html, &len, &alloc,
           "<tr>"
            "<td>"
             "<input type='radio' name='dest' %s value='*'>"
             "<input type='text' name='customdest' value=''>"
            "</td>"
           "</tr>"
          "</table>"
         "</form>", ((checked) ? "checked='true'" : ""));

    rc = doPromptPage(title, html, true, "destination",
                      buttons, locButtons, bcount);

    free(title);
    free(html);
    for (i = 0; i < STATICARRAYLEN(locButtons); i++)
        free((void *) locButtons[i]);

    if (rc == backbutton)
        *command = -1;
    else if (rc == cancelbutton)
        *command = 0;
    else
    {
        const char *dest = NULL;
        const char *customdest = NULL;
        WebRequest *req = webRequest;
        while (req != NULL)
        {
            const char *k = req->key;
            const char *v = req->value;
            if (strcmp(k, "dest") == 0)
                dest = v;
            else if (strcmp(k, "customdest") == 0)
                customdest = v;
            req = req->next;
        } // while

        if (dest != NULL)
        {
            if (strcmp(dest, "*") == 0)
                dest = customdest;
        } // if

        if (dest == NULL)
            *command = 0;   // !!! FIXME: maybe loop with doPromptPage again.
        else
        {
            retval = xstrdup(dest);
            *command = 1;
        } // else
    } // else

    return retval;
} // MojoGui_www_destination


static int MojoGui_www_productkey(const char *desc, const char *fmt,
                                  char *buf, const int buflen,
                                  boolean can_back, boolean can_fwd)
{
    char *prompt = xstrdup(_("Please enter your product key"));
    int retval = -1;
    char *html = NULL;
    size_t len = 0, alloc = 0;
    int cancelbutton = -1;
    int backbutton = -1;
    int fwdbutton = -1;
    int bcount = 0;
    int rc = 0;
    int i = 0;
    const char *buttons[4] = { NULL, NULL, NULL, NULL };
    const char *locButtons[4] = { NULL, NULL, NULL, NULL };
    assert(STATICARRAYLEN(buttons) == STATICARRAYLEN(locButtons));

    cancelbutton = bcount++;
    buttons[cancelbutton] = "cancel";
    locButtons[cancelbutton] = xstrdup(_("Cancel"));

    if (can_back)
    {
        backbutton = bcount++;
        buttons[backbutton] = "back";
        locButtons[backbutton] = xstrdup(_("Back"));
    } // if

    if (can_fwd)
    {
        fwdbutton = bcount++;
        buttons[fwdbutton] = "next";
        locButtons[fwdbutton] = xstrdup(_("Next"));
    } // if

    strAdd(&html, &len, &alloc,
        "<form name='form_productkey' method='get'>"
          "%s<br/>"
          "<input type='text' name='productkey' value='%s'>"
         "</form>", prompt, ((*buf) ? buf : ""));

    free(prompt);

    rc = doPromptPage(desc, html, true, "productkey",
                      buttons, locButtons, bcount);

    free(html);
    for (i = 0; i < STATICARRAYLEN(locButtons); i++)
        free((void *) locButtons[i]);

    if (rc == backbutton)
        retval = -1;
    else if (rc == cancelbutton)
        retval = 0;
    else
    {
        WebRequest *req = webRequest;
        const char *keyval = NULL;
        while (req != NULL)
        {
            const char *k = req->key;
            const char *v = req->value;
            if (strcmp(k, "productkey") == 0)
                keyval = v;

            req = req->next;
        } // while

        if (keyval == NULL)
            retval = 0;   // !!! FIXME: maybe loop with doPromptPage again.
        else
        {
            snprintf(buf, buflen, "%s", keyval);
            if (isValidProductKey(fmt, buf))
                retval = 1;
            // !!! FIXME: must try again if invalid key.
        } // else
    } // else

    return retval;
} // MojoGui_www_productkey


static boolean MojoGui_www_insertmedia(const char *medianame)
{
    char *htmltext = NULL;
    char *text = NULL;
    size_t len = 0, alloc = 0;
    int i, rc;

    const char *buttons[] = { "cancel", "ok" };
    const char *locButtons[] = { htmlescape(_("Cancel")), htmlescape(_("OK")) };

    char *title = xstrdup(_("Media change"));
    char *fmt = xstrdup(_("Please insert '%0'"));
    char *msg = format(fmt, medianame);
    strAdd(&text, &len, &alloc, msg);
    free(msg);
    free(fmt);

    htmltext = htmlescape(text);
    free(text);

    assert(STATICARRAYLEN(buttons) == STATICARRAYLEN(locButtons));

    rc = doPromptPage(title, htmltext, true, "insertmedia", buttons,
                      locButtons, STATICARRAYLEN(buttons));

    free(title);
    free(htmltext);
    for (i = 0; i < STATICARRAYLEN(locButtons); i++)
        free((void *) locButtons[i]);

    return (rc == 1);
} // MojoGui_www_insertmedia


static void MojoGui_www_progressitem(void)
{
    // no-op in this UI target.
} // MojoGui_www_progressitem


static boolean MojoGui_www_progress(const char *type, const char *component,
                                    int percent, const char *item,
                                    boolean can_cancel)
{
    return true;
} // MojoGui_www_progress


static void MojoGui_www_final(const char *msg)
{
    MojoGui_www_msgbox(_("Finish"), msg);
} // MojoGui_www_final

// end of gui_www.c ...

