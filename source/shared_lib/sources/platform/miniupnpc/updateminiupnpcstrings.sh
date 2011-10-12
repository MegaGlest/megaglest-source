#! /bin/sh
# $Id: updateminiupnpcstrings.sh,v 1.7 2011/01/04 11:41:53 nanard Exp $
# project miniupnp : http://miniupnp.free.fr/
# (c) 2009 Thomas Bernard

FILE=miniupnpcstrings.h
TMPFILE=miniupnpcstrings.h.tmp
TEMPLATE_FILE=${FILE}.in

# detecting the OS name and version
OS_NAME=`uname -s`
OS_VERSION=`uname -r`
if [ -f /etc/debian_version ]; then
	OS_NAME=Debian
	OS_VERSION=`cat /etc/debian_version`
fi
# use lsb_release (Linux Standard Base) when available
LSB_RELEASE=`which lsb_release`
if [ 0 -eq $? -a -x "${LSB_RELEASE}" ]; then
	OS_NAME=`${LSB_RELEASE} -i -s`
	OS_VERSION=`${LSB_RELEASE} -r -s`
	case $OS_NAME in
		Debian)
			#OS_VERSION=`${LSB_RELEASE} -c -s`
			;;
		Ubuntu)
			#OS_VERSION=`${LSB_RELEASE} -c -s`
			;;
	esac
fi

# on AmigaOS 3, uname -r returns "unknown", so we use uname -v
if [ "$OS_NAME" = "AmigaOS" ]; then
	if [ "$OS_VERSION" = "unknown" ]; then
		OS_VERSION=`uname -v`
	fi
fi

echo "Detected OS [$OS_NAME] version [$OS_VERSION]"
MINIUPNPC_VERSION=`cat VERSION`
echo "MiniUPnPc version [${MINIUPNPC_VERSION}]"

EXPR="s|OS_STRING \".*\"|OS_STRING \"${OS_NAME}/${OS_VERSION}\"|"
#echo $EXPR
test -f ${FILE}.in
echo "setting OS_STRING macro value to ${OS_NAME}/${OS_VERSION} in $FILE."
sed -e "$EXPR" < $TEMPLATE_FILE > $TMPFILE

EXPR="s|MINIUPNPC_VERSION_STRING \".*\"|MINIUPNPC_VERSION_STRING \"${MINIUPNPC_VERSION}\"|"
echo "setting MINIUPNPC_VERSION_STRING macro value to ${MINIUPNPC_VERSION} in $FILE."
sed -e "$EXPR" < $TMPFILE > $FILE
rm $TMPFILE

