// app.js

import ov_Websocket from "/lib/ov_websocket.js";
import * as CSS from "/css/css.js";
import ov_Player from "/extensions/recorder/components/player/player.js";


export default class ov_Player_List extends HTMLElement {

    // #dom;

    #recordings;
    #uri;

    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
        // this.#dom = {};
    }

    static get observedAttributes() {
        return [];
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue)
            return;
        // if (name === "subtitle")
        //     this.#update_subtitle(newValue);
    }

    async connectedCallback() {
        await this.#render();
        this.draw_recordings(this.#recordings, this.#uri);
        //this.#dom.subtitle = this.shadowRoot.querySelector("#subtitle");
        //this.#update_subtitle(this.getAttribute("subtitle"));
    }

    async #render() {
        this.shadowRoot.adoptedStyleSheets = [await CSS.normalize_style_sheet, await CSS.ov_basic_style_sheet, await ov_Player_List.style_sheet];
        this.shadowRoot.appendChild((await ov_Player_List.template).content.cloneNode(true));
    }

    //set subtitle(text) {
    //    this.setAttribute("subtitle", text);
    //}

    //#/update_subtitle(text) {
    //     if (this.#dom.subtitle) {
    //         this.#dom.subtitle.innerText = text;
    //         this.#dom.subtitle.classList.toggle("unset", !text);
    //     }
    // }

    async draw_recordings(recordings, uri) {
        if (recordings && recordings.length !== 0) {
            this.#recordings = recordings;
            this.#uri = uri;
            
            let players = this.shadowRoot.querySelector("#players");
            players.replaceChildren();

            if (players && recordings) {
                for (let recording of recordings) {
                    let recording_src = uri + recording.uri;
                    console.log(recording_src);
                    console.log(recording);

                    // Create a new ov-player element
                    let player = document.createElement('ov-player');

                    // Set the src and start_epoch_timestamp for the player
                    player.start_epoch_timestamp = recording.starttime;
                    player.src = recording_src;

                    // Append the player to the container
                    players.appendChild(player);
                }
            }
        }
    }

    static style_sheet = CSS.fetch_style_sheet("/extensions/recorder/components/player_list/recorder.css");
    static template = async function () {
        const response = await fetch('/extensions/recorder/components/player_list/recorder.html');
        let dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('template');
    }();
}

customElements.define('ov-player-list', ov_Player_List);