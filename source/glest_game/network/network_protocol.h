// ==============================================================
//	This file is part of MegaGlest (www.megaglest.org)
//
//	Copyright (C) 2012 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef NETWORK_PROTOCOL_H_
#define NETWORK_PROTOCOL_H_

namespace Glest{ namespace Game{

unsigned int pack(unsigned char *buf, const char *format, ...);
unsigned int unpack(unsigned char *buf, const char *format, ...);

}};

#endif /* NETWORK_PROTOCOL_H_ */
