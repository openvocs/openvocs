/***
    ------------------------------------------------------------------------

    Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

            http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    This file is part of the openvocs project. https://openvocs.org

    ------------------------------------------------------------------------
*//**
    @file           ov_audio_activity_detection.js

    @ingroup        ov_lib/ov_media

    @brief          detect if audio stream is muted
    	
    ------------------------------------------------------------------------
*/

// ACTIVITY DETECTION with POWERLEVEL CALC for AUDIO DATA FRAME
export function process_audio_buffer(audio_frame) {
    // CALCULATION OF THE POWER OF THE FRAME
    var power_level = audio_frame.map(item => (item * item / audio_frame.length));

    var sum_power_level = power_level.reduce((res, val) => {
        return res + val;
    });
    
    // 0.0000001 seems to be a good threshold to determine if we have activity 
    // in the frame or not. There is no scientific basis for this value!
    return sum_power_level > 0.0000001;
}