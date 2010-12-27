/*
 * Copyright (c) 2007-2008 Dave Dribin
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


/*
 *  This class is based on CInvocationGrabber:
 *
 *  Copyright (c) 2007, Toxic Software
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  
 *  * Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  
 *  * Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *  
 *  * Neither the name of the Toxic Software nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#import "DDInvocationGrabber.h"


@implementation DDInvocationGrabber

+ (id)invocationGrabber
{
    return([[[self alloc] init] autorelease]);
}

- (id)init
{
    _target = nil;
    _invocation = nil;
    _forwardInvokesOnMainThread = NO;
    _waitUntilDone = NO;
    
    return self;
}

- (void)dealloc
{
    [self setTarget:NULL];
    [self setInvocation:NULL];
    //
    [super dealloc];
}

#pragma mark -

- (id)target
{
    return _target;
}

- (void)setTarget:(id)inTarget
{
    if (_target != inTarget)
	{
        [_target autorelease];
        _target = [inTarget retain];
	}
}

- (NSInvocation *)invocation
{
    return _invocation;
}

- (void)setInvocation:(NSInvocation *)inInvocation
{
    if (_invocation != inInvocation)
	{
        [_invocation autorelease];
        _invocation = [inInvocation retain];
	}
}

- (BOOL)forwardInvokesOnMainThread;
{
    return _forwardInvokesOnMainThread;
}

- (void)setForwardInvokesOnMainThread:(BOOL)forwardInvokesOnMainThread;
{
    _forwardInvokesOnMainThread = forwardInvokesOnMainThread;
}

- (BOOL)waitUntilDone;
{
    return _waitUntilDone;
}

- (void)setWaitUntilDone:(BOOL)waitUntilDone;
{
    _waitUntilDone = waitUntilDone;
}

#pragma mark -

- (NSMethodSignature *)methodSignatureForSelector:(SEL)selector
{
    return [[self target] methodSignatureForSelector:selector];
}

- (void)forwardInvocation:(NSInvocation *)ioInvocation
{
    [ioInvocation setTarget:[self target]];
    [self setInvocation:ioInvocation];
    if (_forwardInvokesOnMainThread)
    {
        if (!_waitUntilDone)
            [_invocation retainArguments];
        [_invocation performSelectorOnMainThread:@selector(invoke)
                                      withObject:nil
                                   waitUntilDone:_waitUntilDone];
    }
}

@end

#pragma mark -

@implementation DDInvocationGrabber (DDnvocationGrabber_Conveniences)

- (id)prepareWithInvocationTarget:(id)inTarget
{
    [self setTarget:inTarget];
    return(self);
}

@end
