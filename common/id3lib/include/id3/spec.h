// $Id: spec.h,v 1.1 2002/01/21 08:16:21 menno Exp $

// id3lib: a software library for creating and manipulating id3v1/v2 tags
// Copyright 1999, 2000  Scott Thomas Haug

// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
// License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// The id3lib authors encourage improvements and optimisations to be sent to
// the id3lib coordinator.  Please see the README file for details on where to
// send such submissions.  See the AUTHORS file for a list of people who have
// contributed to id3lib.  See the ChangeLog file for a list of changes to
// id3lib.  These files are distributed with id3lib at
// http://download.sourceforge.net/id3lib/

#ifndef __ID3LIB_SPEC_H__
#define __ID3LIB_SPEC_H__

#include "globals.h"

ID3_V2Spec ID3_VerRevToV2Spec(uchar ver, uchar rev);
uchar      ID3_V2SpecToVer(ID3_V2Spec spec);
uchar      ID3_V2SpecToRev(ID3_V2Spec spec);

class ID3_Speccable
{
public:
  virtual bool       SetSpec(ID3_V2Spec) = 0;
  virtual ID3_V2Spec GetSpec() const = 0;

  /* The following methods are deprecated */
  virtual bool       SetVersion(uchar ver, uchar rev)
  {
    return this->SetSpec(ID3_VerRevToV2Spec(ver, rev));
  }
  virtual uchar      GetVersion() const
  {
    return ID3_V2SpecToVer(this->GetSpec());
  }
  virtual uchar      GetRevision() const
  {
    return ID3_V2SpecToRev(this->GetSpec());
  }
};

#endif /* __ID3LIB_SPEC_H__ */
