/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: sbr_dec.c,v 1.3 2002/04/23 21:08:26 menno Exp $
**/

/*
   SBR Decoder overview:

   To achieve a synchronized output signal, the following steps have to be
   acknowledged in the decoder:
    - The bitstream parser divides the bitstream into two parts; the AAC
      core coder part and the SBR part.
    - The SBR bitstream part is fed to the bitstream de-multiplexer followed
      by de-quantization The raw data is Huffman decoded.
    - The AAC bitstream part is fed to the AAC core decoder, where the
      bitstream data of the current frame is decoded, yielding a time domain
      audio signal block of 1024 samples. The block length could easily be
      adapted to other sizes e.g. 960.
    - The core coder audio block is fed to the analysis QMF bank using a
      delay of 1312 samples.
    - The analysis QMF bank performs the filtering of the delayed core coder
      audio signal. The output from the filtering is stored in the matrix
      Xlow. The output from the analysis QMF bank is delayed tHFGen subband
      samples, before being fed to the synthesis QMF bank. To achieve
      synchronization tHFGen = 32, i.e. the value must equal the number of
      subband samples corresponding to one frame.
    - The HF generator calculates XHigh given the matrix XLow. The process
      is guided by the SBR data contained in the current frame.
    - The envelope adjuster calculates the matrix Y given the matrix XHigh
      and the SBR envelope data, extracted from the SBR bitstream. To
      achieve synchronization, tHFAdj has to be set to tHFAdj = 0, i.e. the
      envelope adjuster operates on data delayed tHFGen subband samples.
    - The synthesis QMF bank operates on the delayed output from the analysis
      QMF bank and the output from the envelope adjuster.
 */

#include "common.h"

#ifdef SBR

#include "sbr_syntax.h"
#include "sbr_qmf.h"

#endif
