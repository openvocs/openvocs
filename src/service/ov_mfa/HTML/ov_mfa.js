/*
 * 	ov_mfa.js
 * 	Author: Markus Töpfer
 *
 *
 *      This JS file expects some ov_websocket with a configure function,
 *      to set itself as protocol for the websocket to consume incoming messages.
 *
 *      ------------------------------------------------------------------------
 *
 *      #GLOBAL Variables
 *
 *      ------------------------------------------------------------------------
 */

// Convert base64url string to Uint8Array (ArrayBuffer)
function base64urlToBuffer(base64url) {
    // Convert base64url to base64
    let base64 = base64url.replace(/-/g, '+').replace(/_/g, '/');
    // Add padding if necessary
    while (base64.length % 4) {
        base64 += '=';
    }
    let binaryString = atob(base64);
    let bytes = new Uint8Array(binaryString.length);
    for (let i = 0; i < binaryString.length; i++) {
        bytes[i] = binaryString.charCodeAt(i);
    }
    return bytes.buffer;
}

// Convert ArrayBuffer to base64url string (needed when sending data back to C server)
function bufferToBase64url(buffer) {
    let bytes = new Uint8Array(buffer);
    let binary = '';
    for (let i = 0; i < bytes.byteLength; i++) {
        binary += String.fromCharCode(bytes[i]);
    }
    let base64 = btoa(binary);
    return base64.replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
}


var ov_mfa = {

    debug: true,
    session: null,
    client: null,

    create_uuid: function() {
        function s4() {
            return Math.floor((1 + Math.random()) * 0x10000).toString(16).substring(1);
        }
        return s4() + s4() + "-" + s4() + "-" + s4() + "-" + s4() + "-" + s4() + s4() + s4();
    },

    init: function(){

        if (null == ov_websocket){

            console.error("ov_mfa is a protocol for ov_websocket " +
                "some ov_websocket MUST be present!");

        } else {

            console.log("ov_mfa.incoming set as ov_websocket.incoming.")
            ov_websocket.incoming = ov_mfa.incoming;

            ov_websocket.debug(this.debug);

        }

        this.client = this.create_uuid();

        ov_websocket.connect(null);

    },

    incoming: function(msg){

        /* Expect some JSON message input. */

        if (true == this.debug)
            console.log("<-- RECV "+ JSON.stringify(msg, null, 2));

        if (msg.response.challenge)
            ov_mfa.process_challenge(msg.response.challenge);

        if (msg.response.mfa)
            ov_mfa.process_mfa(msg.response.mfa);

    },

    register: function(username){

        let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "register",
            parameter: {}
        }

        request.parameter.user = username;

        ov_websocket.send(JSON.stringify(request));
    },

    login: function(username, password){

        let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "login",
            parameter: {}
        }

        request.parameter.user = username;
        request.parameter.password = password;

        ov_websocket.send(JSON.stringify(request));
    },

    process_challenge: async function (options) {

        options.challenge = base64urlToBuffer(options.challenge);
        options.user.id = base64urlToBuffer(options.user.id);

        // Prompt authenticator (TouchID, Windows Hello, YubiKey)
        let credential = await navigator.credentials.create({ publicKey: options });

        // 3. Package response components to send back to your C server
        let credentialPayload = {
            id: credential.id,
            rawId: bufferToBase64url(credential.rawId),
            type: credential.type,
            response: {
                clientDataJSON: bufferToBase64url(credential.response.clientDataJSON),
                attestationObject: bufferToBase64url(credential.response.attestationObject)
            }
        };

        let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "mfa_register",
            parameter: {}
        }

        request.parameter = credentialPayload;
        ov_websocket.send(JSON.stringify(request));
    },

    process_mfa: async function (options) {

        options.challenge = base64urlToBuffer(options.challenge);
        options.allowCredentials.forEach(c => {
            c.id = base64urlToBuffer(c.id);
        });

        // Authenticator (YubiKey, TouchID, etc.) auffordern, die Challenge zu signieren
        let assertion = await navigator.credentials.get({ publicKey: options });

        let assertionPayload = {
            id: assertion.id,
            rawId: bufferToBase64url(assertion.rawId),
            type: assertion.type,
            response: {
                clientDataJSON: bufferToBase64url(assertion.response.clientDataJSON),
                authenticatorData: bufferToBase64url(assertion.response.authenticatorData),
                signature: bufferToBase64url(assertion.response.signature),
                userHandle: assertion.response.userHandle ? bufferToBase64url(assertion.response.userHandle) : null
            }
        };

        let request = {
            uuid : this.create_uuid(),
            client: this.client,
            event: "mfa_login",
            parameter: {}
        }

        request.parameter = assertionPayload;
        ov_websocket.send(JSON.stringify(request));
    }
};

ov_mfa.init();