export default class ConnectionConfigDTO {
    constructor(params = {}) {
        const { host = "", port = 0, version = 0, flags = 15, username = "", password = "" } = params;

        this.host = host;
        this.port = port;
        this.version = version;
        this.flags = flags;
        this.username = username;
        this.password = password;
    }
}
