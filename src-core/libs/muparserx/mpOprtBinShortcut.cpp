/*
			   __________                                 ____  ___
	_____  __ _\______   \_____ _______  ______ __________\   \/  /
   /     \|  |  \     ___/\__  \\_  __ \/  ___// __ \_  __ \     /
  |  Y Y  \  |  /    |     / __ \|  | \/\___ \\  ___/|  | \/     \
  |__|_|  /____/|____|    (____  /__|  /____  >\___  >__| /___/\  \
		\/                     \/           \/     \/           \_/
  Copyright (C) 2023 Ingo Berg, et al.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice,
	 this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice,
	 this list of conditions and the following disclaimer in the documentation
	 and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
#include "mpOprtBinShortcut.h"

MUP_NAMESPACE_START
//-----------------------------------------------------------------------------------------------
//
// class OprtShortcutLogicOrBegin
//
//-----------------------------------------------------------------------------------------------

OprtShortcutLogicOrBegin::OprtShortcutLogicOrBegin(const char_type* szIdent)
	:IOprtBinShortcut(cmSHORTCUT_BEGIN, szIdent, (int)prLOGIC_OR, oaLEFT)
{}


//-----------------------------------------------------------------------------------------------
IToken* OprtShortcutLogicOrBegin::Clone() const
{
	return new OprtShortcutLogicOrBegin(*this);
}


//-----------------------------------------------------------------------------------------------
//
// class OprtShortcutLogicOrEnd
//
//-----------------------------------------------------------------------------------------------

OprtShortcutLogicOrEnd::OprtShortcutLogicOrEnd(const char_type* szIdent)
	:IOprtBinShortcut(cmSHORTCUT_END, szIdent, (int)prLOGIC_OR, oaLEFT)
{}


//-----------------------------------------------------------------------------------------------
IToken* OprtShortcutLogicOrEnd::Clone() const
{
	return new OprtShortcutLogicOrEnd(*this);
}

//-----------------------------------------------------------------------------------------------
//
// class OprtShortcutLogicAndBegin
//
//-----------------------------------------------------------------------------------------------

OprtShortcutLogicAndBegin::OprtShortcutLogicAndBegin(const char_type* szIdent)
	:IOprtBinShortcut(cmSHORTCUT_BEGIN, szIdent, (int)prLOGIC_AND, oaLEFT)
{}

//-----------------------------------------------------------------------------------------------
IToken* OprtShortcutLogicAndBegin::Clone() const
{
	return new OprtShortcutLogicAndBegin(*this);
}


//-----------------------------------------------------------------------------------------------
//
// class OprtShortcutLogicAndEnd
//
//-----------------------------------------------------------------------------------------------

OprtShortcutLogicAndEnd::OprtShortcutLogicAndEnd(const char_type* szIdent)
	:IOprtBinShortcut(cmSHORTCUT_END, szIdent, (int)prLOGIC_AND, oaLEFT)
{}

//-----------------------------------------------------------------------------------------------
IToken* OprtShortcutLogicAndEnd::Clone() const
{
	return new OprtShortcutLogicAndEnd(*this);
}

}