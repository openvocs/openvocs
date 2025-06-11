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

    ---------------------------------------------------------------------------
*//**
    @file           ov_audio_visualization.js
        
    @author         Anja Bertard, Fabian Junge

    @date           2019-10-04

    @ingroup        ov_lib/ov_media

    @brief          visualize an audio stream
    	
    ---------------------------------------------------------------------------
*/

const ANALYSER_FFT_SIZE = 512;

const STYLE_DEFAULT_LINE_WIDTH = 1.5;
const STYLE_DEFAULT_LINE = "#000000";

export default class ov_Audio_Visualization {
    /**
    * @constructor
    * @param {Object} element - HTML Canvas Element
    * @param {Object} source_node
    */

    #context;
    #waveform_color;
    #analyser;
    #source

    constructor(element) {
        if (!element)
            console.error("(ov) audio vis: no DOM element passed in constructor");

        this.#context = element.getContext("2d");
        this.#context.fillStyle = this.#waveform_color;
        this.#context.lineWidth = STYLE_DEFAULT_LINE_WIDTH;
        this.#context.canvas.width = element.offsetWidth;
        this.#context.canvas.height = element.offsetHeight;

        this.waveform_color = STYLE_DEFAULT_LINE;
    }

    start(source_node, audio_context) {
        if (!this.#analyser) {
            this.#analyser = audio_context.createAnalyser();
            this.#analyser.fftSize = ANALYSER_FFT_SIZE;

            this.#source = source_node;
            this.#source.connect(this.#analyser);
        }

        // for every pixel in width one waveform value
        let waveform_data = new Uint8Array(ANALYSER_FFT_SIZE);

        this.#draw(waveform_data);
    }


    #draw(waveform_data) {
        requestAnimationFrame(() => { this.#draw(waveform_data) });

        this.#context.clearRect(0, 0, this.#context.canvas.width,
            this.#context.canvas.height);

        this.#analyser.getByteTimeDomainData(waveform_data);

        let mid_y = this.#context.canvas.height / 2;
        let draw_step_size_x = this.#context.canvas.width / ANALYSER_FFT_SIZE;
        let current_draw_x = 0;

        this.#context.strokeStyle = this.#waveform_color;

        this.#context.beginPath();
        for (let i = 0; i < ANALYSER_FFT_SIZE; i++) {
            let current_draw_y = (waveform_data[i] / 128.0) * mid_y;

            if (i === 0) {
                this.#context.moveTo(current_draw_x, current_draw_y);
            } else {
                this.#context.lineTo(current_draw_x, current_draw_y);
            }
            current_draw_x += draw_step_size_x;
        }

        this.#context.lineTo(this.#context.canvas.width, mid_y);
        this.#context.stroke();
    }

    stop() {
        this.#source.disconnect(this.#analyser);
    }

    set waveform_color(color) {
        this.#waveform_color = color;
    }
}