#!/bin/sh

echo "static struct fetcherr _http_errlist[] = {" > httperr.h
cat http.errors \
          | grep -v ^# \
          | sort \
          | while read NUM CAT STRING; do \
            echo "    { ${NUM}, FETCH_${CAT}, \"${STRING}\" },"; \
          done >> httperr.h
echo "    { -1, FETCH_UNKNOWN, \"Unknown HTTP error\" }" >> httperr.h
echo "};" >> httperr.h
echo >> httpderr.h

echo "static struct fetcherr _ftp_errlist[] = {" > ftperr.h
cat ftp.errors \
          | grep -v ^# \
          | sort \
          | while read NUM CAT STRING; do \
            echo "    { ${NUM}, FETCH_${CAT}, \"${STRING}\" },"; \
          done >> ftperr.h
echo "    { -1, FETCH_UNKNOWN, \"Unknown FTP error\" }" >> ftperr.h
echo "};" >> ftperr.h
echo >> ftperr.h


