import * as CSS from "/css/css.js";

export default class ov_Player extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
        this.fakeDuration = null; // Property to store the fake duration
        this.startEpochTimestamp = null; // Property for the start timestamp
    }

    static get observedAttributes() {
        return ["src"];
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if (oldValue === newValue) return;
        if (name === "src") this.#update_src(newValue);
    }

    async connectedCallback() {
        // Render the template into the shadow DOM
        await this.#render();

        // Initialize elements
        this.#initializeElements();

        // Ensure essential elements exist
        if (!this.audioElement || !this.playPauseButton || !this.progressContainer || !this.progressBar || !this.startTimeDisplay) {
            console.error('Missing essential elements!');
            return;
        }

        // Update the `src` attribute if it's set
        const src = this.getAttribute("src");
        this.#update_src(src);

        // Render start time if available
        if (this.startEpochTimestamp && this.startTimeDisplay) {
            const startDate = new Date(this.startEpochTimestamp * 1000); // Convert epoch to date
            this.startTimeDisplay.textContent = startDate.toLocaleString(); // Format as readable date
        }

        // Add event listeners for play/pause, progress update, and seeking
        this.playPauseButton.addEventListener('click', this.togglePlayPause.bind(this));
        this.progressContainer.addEventListener('click', this.seekAudio.bind(this));
        this.audioElement.addEventListener('timeupdate', this.updateElapsedTime.bind(this));

        // Handle metadata loading to fetch duration
        this.audioElement.addEventListener('loadedmetadata', async () => {
            const duration = isFinite(this.audioElement.duration)
                ? this.audioElement.duration
                : this.fakeDuration || 0;

            console.log('Audio duration:', duration);
            if (this.totalTimeDisplay) {
                this.totalTimeDisplay.textContent = this.formatTime(duration);
            }
        });
    }

    async #render() {
        const template = await ov_Player.template;
        if (template) {
            this.shadowRoot.appendChild(template.content.cloneNode(true));
        } else {
            console.error('Template not found or failed to load.');
        }

        // Apply CSS styles
        this.shadowRoot.adoptedStyleSheets = [
            await CSS.normalize_style_sheet,
            await CSS.ov_basic_style_sheet,
            await ov_Player.style_sheet
        ];
    }

    #initializeElements() {
        // Ensure elements exist in the shadow root
        this.audioElement = this.shadowRoot.querySelector('#my-audio');
        this.playPauseButton = this.shadowRoot.querySelector('#recorder-play-button button');
        this.elapsedTimeDisplay = this.shadowRoot.querySelector('#elapsed-time');
        this.totalTimeDisplay = this.shadowRoot.querySelector('#total-time'); // Added reference
        this.progressContainer = this.shadowRoot.querySelector('#progress-container');
        this.progressBar = this.shadowRoot.querySelector('#progress-bar');
        this.startTimeDisplay = this.shadowRoot.querySelector('#start-timestamp');

        // Check if all required elements are found
        if (!this.audioElement || !this.playPauseButton || !this.elapsedTimeDisplay ||
            !this.totalTimeDisplay || !this.progressContainer || !this.progressBar) {
            console.error('One or more required elements are missing in the shadow DOM.');
            return;
        }
    }

    #update_src(src) {
        const audioSource = this.shadowRoot.querySelector("#my-audio > source");
        if (audioSource) {
            audioSource.src = src;
            this.audioElement.load();
        }
    }

    set src(src) {
        this.setAttribute("src", src);
    }

    get src() { return this.getAttribute("src"); }

    // Set the start epoch timestamp
    set start_epoch_timestamp(timestamp) {
        this.startEpochTimestamp = timestamp;
    }

    // Get the start epoch timestamp
    get start_epoch_timestamp() {
        return this.startEpochTimestamp;
    }

    static style_sheet = CSS.fetch_style_sheet("/extensions/recorder/components/player/player.css");
    static template = async function () {
        const response = await fetch('/extensions/recorder/components/player/player.html');
        const dom = new DOMParser().parseFromString(await response.text(), 'text/html');
        return dom.querySelector('#template');
    }();

    togglePlayPause() {
        const playIcon = this.shadowRoot.querySelector('#play-icon');
        const pauseIcon = this.shadowRoot.querySelector('#pause-icon');

        if (this.audioElement.paused) {
            this.audioElement.play();
            if (playIcon) playIcon.style.display = 'none';
            if (pauseIcon) pauseIcon.style.display = 'inline';
        } else {
            this.audioElement.pause();
            if (playIcon) playIcon.style.display = 'inline';
            if (pauseIcon) pauseIcon.style.display = 'none';
        }
    }

    updateElapsedTime() {
        const currentTime = this.audioElement.currentTime;
        const duration = this.fakeDuration || this.audioElement.duration;

        if (!isFinite(duration)) {
            console.log('Duration is infinite or unavailable. Progress bar will not update.');
            this.progressBar.style.width = '100%'; // For live streams, always show full bar
            this.elapsedTimeDisplay.textContent = 'Live';
            return;
        }

        const progressPercentage = (currentTime / duration) * 100;
        this.progressBar.style.width = `${Math.max(progressPercentage, 1)}%`; // Ensure minimum width
        this.elapsedTimeDisplay.textContent = this.formatTime(currentTime);

        if (this.totalTimeDisplay) {
            this.totalTimeDisplay.textContent = this.formatTime(duration); // Update total duration
        }

        console.log('Updating progress:', {
            currentTime,
            duration,
            progressPercentage,
            barWidth: this.progressBar.style.width,
        });
    }

    seekAudio(event) {
        const rect = this.progressContainer.getBoundingClientRect();
        const clickX = event.clientX - rect.left;
        const containerWidth = rect.width;
        const duration = this.fakeDuration || this.audioElement.duration;

        if (!isFinite(duration)) {
            console.log('Cannot seek on a live stream or infinite duration. Using fake duration.');
        }

        const newTime = (clickX / containerWidth) * duration;
        this.audioElement.currentTime = newTime;

        console.log('Seeking to:', {
            clickX,
            containerWidth,
            duration,
            newTime,
        });
    }

    formatTime(seconds) {
        const minutes = Math.floor(seconds / 60);
        const secs = Math.floor(seconds % 60).toString().padStart(2, '0');
        return `${minutes}:${secs}`;
    }
}

customElements.define('ov-player', ov_Player);