/*
 * Copyright 2011-2019, European Organisation for the Exploitation of Meteorological Satellites (EUMETSAT)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WTConst_included
#define WTConst_included

namespace COMP
{

    const unsigned int c_ACNbBits = 31; // size in bits of the arithmetic coding range

    // Marker values
    const unsigned __int16 c_MarkerHEADER = 0xFF01;
    const unsigned __int16 c_MarkerDATA = 0xFF02;
    const unsigned __int16 c_MarkerFOOTER = 0xFF03;
    const unsigned __int16 c_MarkerRESTART = 0xFFE0;

} // end namespace

#endif
