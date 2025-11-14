/***
    ---------------------------------------------------------------------------

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

    ---------------------------------------------------------------------------
*//**
    @file           ov_audio.js

    @ingroup        ov_media

    @brief          utils for media stream initialization + handling
    	
    ---------------------------------------------------------------------------
*/
import * as ov_Audio_Activity_Detection from "./ov_audio_activity_detection.js";
import * as ov_Voice_Activity_Detection from "./ov_voice_activity_detection.js";
import ov_Audio_Visualization from "./ov_audio_visualization.js";
// Global Music Mode flag (client-side only)
if (typeof window !== "undefined") {
    try {
        const saved = window.localStorage.getItem("ov_music_mode");
        window.ov_music_mode = saved === "1";
    } catch (e) {
        window.ov_music_mode = false;
    }
}

export default class ov_Audio {
    static #audio_context;

    #stream;
    #source;
    #analyser_node;

    #current_activity;
    #current_vad;

    #audio_activity_callback;
    #voice_activity_callback;

    constructor(stream, audio_context) {
        ov_Audio.#audio_context = audio_context ? audio_context : ov_Audio.audio_context();
        this.#stream = stream;
        this.#source = ov_Audio.#audio_context.createMediaStreamSource(stream);
        this.#analyser_node = ov_Audio.#audio_context.createAnalyser();
    }

    get stream() {
        return this.#stream;
    }

    get mute() {
        return !this.#stream.getAudioTracks()[0].enabled;
    }

    set mute(value) {
        this.#stream.getAudioTracks()[0].enabled = !value;
    }

    //-------------------------------------------------------------------------
    // callbacks to inform about changes in audio stream
    //-------------------------------------------------------------------------
    set on_activity_detection(callback) {
        if (typeof callback !== "function")
            callback = undefined;
        this.#audio_activity_callback = callback;
    }

    set on_voice_activity_detection(callback) {
        if (typeof callback !== "function")
            callback = undefined;
        this.#voice_activity_callback = callback;
    }

    //-------------------------------------------------------------------------
    // analyses audio stream
    //-------------------------------------------------------------------------
    // frequency analyses -----------------------------------------------------
    start_activity_analyses() {
        this.#analyser_node.fftSize = 16384;
        this.#analyser_node.smoothingTimeConstant = 0;

        this.#source.connect(this.#analyser_node);

        this.#activity_analyse();
        setInterval(() => {
            this.#activity_analyse();
        }, 250);
    }

    get current_activity() {
        return this.#current_activity;
    }

    stop_activity_analyses() {
        try {
            this.#source.disconnect(this.#analyser_node);
        } catch (e) { }
    }

    #activity_analyse() {
        var input_frame = new Float32Array(this.#analyser_node.frequencyBinCount);
        this.#analyser_node.getFloatTimeDomainData(input_frame);

        if (this.#audio_activity_callback) {
            let activity = ov_Audio_Activity_Detection.process_audio_buffer(input_frame);
            if (activity !== this.#current_activity) {
                this.#current_activity = activity;
                if (this.#audio_activity_callback)
                    this.#audio_activity_callback(activity);
            }
        }

        if (this.#voice_activity_callback) {
            let vad = ov_Voice_Activity_Detection.process_audio_buffer(input_frame);
            if (vad !== this.#current_vad) {
                this.#current_vad = vad;
            }
        }
    }

    // audio vis --------------------------------------------------------------
    create_visualization(element, color) {
        let vis = new ov_Audio_Visualization(element);
        if (color)
            vis.waveform_color = color;
        vis.start(this.#source, ov_Audio.#audio_context);
        return vis;
    }

    //-------------------------------------------------------------------------
    // static methods to create ov_Audio
    //-------------------------------------------------------------------------
    static oscillator_media_stream(type, frequency) {
        let audio_context = ov_Audio.audio_context();
        let source = audio_context.createOscillator();
        let dest = audio_context.createMediaStreamDestination();

        source.connect(dest);

        source.type = type;
        source.frequency.value = frequency; // value in Hz
        source.start();

        return new ov_Audio(dest.stream, audio_context);
    }

    static media_stream_from_file(path) {
        let audio_context = ov_Audio.audio_context();
        let source;
        let dest = audio_context.createMediaStreamDestination();
        let request = new XMLHttpRequest();
        request.open("GET", path, true);
        request.responseType = "arraybuffer";
        request.onload = function () {
            let audioData = request.response;
            audio_context.decodeAudioData(audioData, function (buffer) {
                source = audio_context.createBufferSource();
                source.buffer = buffer;
                source.loop = true;
                source.connect(dest);
                source.start();
            }, console.error);
        }
        request.send();
        return new ov_Audio(dest.stream, audio_context);
    }

   static async media_stream_from_microphone() {
    console.log("(ov) audio: Ask for local audio stream");

    const music = (typeof window !== "undefined") && window.ov_music_mode === true;

    const base = {
        channelCount: 2,
        sampleRate: 48000
    };

    const processingOff = {
        echoCancellation: false,
        noiseSuppression: false,
        autoGainControl: false,
        typingNoiseDetection: false,  // some browsers
        voiceIsolation: false,        // Safari / some Chromium
        googEchoCancellation: false,  // legacy flags
        googNoiseSuppression: false,
        googAutoGainControl: false
    };

    const processingOn = {
        echoCancellation: true,
        noiseSuppression: true,
        autoGainControl: true
    };

    const constraints = {
        audio: {
            ...base,
            ...(music ? processingOff : processingOn)
        },
        video: false
    };

    console.log("(ov) audio: Constraints:", constraints);

    const stream = await navigator.mediaDevices.getUserMedia(constraints);

    console.log("(ov) audio: Got local audio stream (music mode:", music, ")");

    return new ov_Audio(stream, ov_Audio.audio_context());
}

static async reload_microphone_for_peer(pc, options = {}) {
    if (!pc) {
        console.warn("(ov) audio: reload_microphone_for_peer called without pc");
        return null;
    }

    const monitorElementId = options.monitorElementId || "ov_local_mic_monitor";

    // Get a fresh ov_Audio with current Music Mode
    const audioObj = await ov_Audio.media_stream_from_microphone();

    // If your constructor stores the stream differently, adjust this:
    const stream = audioObj.stream || audioObj.media_stream || audioObj;
    const track = stream.getAudioTracks()[0];

    if (!track) {
        console.error("(ov) audio: no audio track in new microphone stream");
        return null;
    }

    const sender = pc.getSenders().find(s => s.track && s.track.kind === "audio");

    if (sender) {
        await sender.replaceTrack(track);
        console.log("(ov) audio: Replaced audio track on existing sender");
    } else {
        pc.addTrack(track, stream);
        console.log("(ov) audio: Added audio track to peer connection");
    }

    // Optional: local muted monitor
    if (typeof document !== "undefined") {
        const el = document.getElementById(monitorElementId);
        if (el) {
            el.srcObject = stream;
            el.muted = true;
            el.play().catch(() => {});
        }
    }

    return audioObj;
}


    static clone(ov_audio){
        return new ov_Audio(ov_audio.stream.clone(), ov_Audio.audio_context());
    }

    //-------------------------------------------------------------------------
    // audio context
    //-------------------------------------------------------------------------
    static audio_context() {
        if (!ov_Audio.#audio_context)
            ov_Audio.#audio_context = new window.AudioContext();
        return ov_Audio.#audio_context;
    }
};
