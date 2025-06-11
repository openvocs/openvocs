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
    @file           ov_voice_activity_detection.js

    @ingroup        ov_lib/ov_media

    @brief          detect voice activity in audio stream
    	
    ------------------------------------------------------------------------
*/

// DEF POWERLEVEL CALC FOR AUDIO DATA FRAME AS level_calc
function level_calc(audio_frame) {
    var threshold_power_level = -60;  // TODO ENTER CORRECT VALUE AFTER CHECKING THE MIC input
    // CALCULATION OF THE POWER OF THE FRAME IN dB
    var power_level = audio_frame.map(item => (item * item / audio_frame.length));
    var sum_power_level = power_level.reduce((res, val) => {
        return res + val;
    });
    var power_level_dB = 20 * Math.log10(sum_power_level);
    //console.log(power_level_dB)
    let vad_power_level = false;
    if (power_level_dB >= threshold_power_level)
        vad_power_level = true;
    return vad_power_level;
}


// DEF. ZEROCROSSING AS ZC
function zc(audio_frame) {
    var threshold_zc = 2000;
    var zc_array;
    (zc_array = []).length = 1024;
    zc_array.fill(0);
    for (let idx = 0; idx < audio_frame.length; idx++) {
        if ((audio_frame[idx] >= 0 && audio_frame[idx - 1] < 0)
            ||
            (audio_frame[idx] < 0 && audio_frame[idx - 1] >= 0)) {
            zc_array[idx] = 1;
        } else {
            zc_array[idx] = 0;
        };
    };
    const zc_count = zc_array.reduce((result, number) => result + number);
    //var threshold_zc_check = (zc_count <= threshold_zc) ? 'Speech' : 'Noise'; // 200 #160 = 8kHz #200 = 10kHz
    var threshold_zc_check = (zc_count <= threshold_zc) ? true : false;
    return threshold_zc_check
};

// DEF. FUNCTION FOR VAD DECISION
export function process_audio_buffer(microphone_output_buffer) { // invoked by event loop

    // VAD FEATURE CALCULATION:
    let vad_zc = zc(microphone_output_buffer);
    let vad_power_level = level_calc(microphone_output_buffer);

    // VAD SPEECH / NOISE
    return vad_zc && vad_power_level;
}