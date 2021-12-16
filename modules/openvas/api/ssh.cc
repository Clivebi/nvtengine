#include <libssh/libssh.h>

#include "../../net/con.hpp"
#include "../api.hpp"

class SSHSession : public Resource {
protected:
    DISALLOW_COPY_AND_ASSIGN(SSHSession);
    int methods;
    int hasSetUserName;

public:
    scoped_refptr<Resource> mCon;
    ssh_session mSession;

    explicit SSHSession(scoped_refptr<Resource> con) : mCon(con) {
        mSession = ssh_new();
        ssh_init();
        methods = 0;
        hasSetUserName = 0;
        int nValue = 1;
        ssh_options_set(mSession, SSH_OPTIONS_LOG_VERBOSITY, &nValue);
    }
    bool SetUserName(std::string name) {
        if (hasSetUserName) {
            return true;
        }
        if (!name.size()) {
            return false;
        }
        if (ssh_options_set(mSession, SSH_OPTIONS_USER, name.c_str())) {
            return false;
        }
        hasSetUserName = true;
        return true;
    }
    int GetAuthMethod() {
        if (this->methods) {
            return this->methods;
        }
        if (!hasSetUserName) {
            return 0;
        }
        int rc = ssh_userauth_none(mSession, NULL);
        if (rc == SSH_AUTH_SUCCESS) {
            this->methods = SSH_AUTH_METHOD_NONE;
        } else if (rc == SSH_AUTH_DENIED) {
            this->methods = ssh_userauth_list(mSession, NULL);
        } else {
            this->methods =
                    (SSH_AUTH_METHOD_NONE | SSH_AUTH_METHOD_PASSWORD | SSH_AUTH_METHOD_PUBLICKEY |
                     SSH_AUTH_METHOD_HOSTBASED | SSH_AUTH_METHOD_INTERACTIVE);
        }
        return this->methods;
    }
    virtual void Close() {
        mCon = NULL;
        if (mSession) {
            ssh_disconnect(mSession);
            ssh_free(mSession);
        }
        mSession = NULL;
    };
    virtual bool IsAvaliable() { return mCon != NULL; }
    virtual std::string TypeName() { return "SSH"; };
};

class SSHChannel : public Resource {
protected:
    DISALLOW_COPY_AND_ASSIGN(SSHChannel);

public:
    ssh_channel mChannel;
    scoped_refptr<SSHSession> mSession;
    explicit SSHChannel(scoped_refptr<SSHSession> session) : mSession(session) {
        mChannel = ssh_channel_new(mSession->mSession);
    }

    bool Open() { return 0 == ssh_channel_open_session(mChannel); }
    bool Shell(int pty) {
        if (pty == 1) {
            if (ssh_channel_request_pty(mChannel)) {
                return false;
            }

            if (ssh_channel_change_pty_size(mChannel, 80, 24)) {
                return false;
            }
        }
        if (ssh_channel_request_shell(mChannel)) {
            return false;
        }
        return true;
    }

    int readNB(std::string& result) {
        int rc;
        char buffer[4096];

        if (!ssh_channel_is_open(mChannel) || ssh_channel_is_eof(mChannel)) return -1;

        while ((rc = ssh_channel_read_nonblocking(mChannel, buffer, sizeof(buffer), 1)) > 0)
            result.append(buffer, rc);
        if (rc == SSH_ERROR) return -1;
        while ((rc = ssh_channel_read_nonblocking(mChannel, buffer, sizeof(buffer), 0)) > 0)
            result.append(buffer, rc);
        if (rc == SSH_ERROR) return -1;
        return 0;
    }

    int write(const std::string& data) {
        return ssh_channel_write(mChannel, data.c_str(), data.size());
    }

    virtual void Close() {
        if (mChannel) {
            ssh_channel_free(mChannel);
            mChannel = NULL;
        }
        mSession = NULL;
    };
    virtual bool IsAvaliable() { return mSession != NULL; }
    virtual std::string TypeName() { return "SSH-Channel"; };
};

//socket,timeout,ip,keytype,cschiphers,sscriphers
Value SSHConnect(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_INTEGER(1);
    CHECK_PARAMETER_STRING(2);
    int timeout = GetInt(args, 1, 15);
    std::string ip = GetString(args, 2);
    std::string keyType = GetString(args, 3);
    std::string csciphers = GetString(args, 4);
    std::string sscriphers = GetString(args, 5);
    scoped_refptr<SSHSession> session = new SSHSession(args[0].resource);
    ssh_options_set(session->mSession, SSH_OPTIONS_SSH_DIR, "./");
    ssh_options_set(session->mSession, SSH_OPTIONS_TIMEOUT, &timeout);
    ssh_options_set(session->mSession, SSH_OPTIONS_HOST, ip.c_str());
    ssh_options_set(session->mSession, SSH_OPTIONS_KNOWNHOSTS, "/dev/null");
    if (keyType.size()) {
        ssh_options_set(session->mSession, SSH_OPTIONS_HOSTKEYS, keyType.c_str());
    }
    if (csciphers.size()) {
        ssh_options_set(session->mSession, SSH_OPTIONS_CIPHERS_C_S, csciphers.c_str());
    }
    if (sscriphers.size()) {
        ssh_options_set(session->mSession, SSH_OPTIONS_CIPHERS_S_C, sscriphers.c_str());
    }
    net::Conn* con = (net::Conn*)args[0].resource.get();
    int FD = con->GetBaseConn()->GetFD();
    ssh_options_set(session->mSession, SSH_OPTIONS_FD, &FD);
    if (ssh_connect(session->mSession)) {
        LOG(ssh_get_error(session->mSession));
        return Value();
    }
    return Value((Resource*)session);
}

//session,username,password,privatekey,passphrase
Value SSHAuth(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(5);
    CHECK_PARAMETER_RESOURCE(0);
    std::string username = GetString(args, 1);
    std::string password = GetString(args, 2);
    std::string privateKey = GetString(args, 3);
    std::string passphrase = GetString(args, 4);
    int rc = 0, methods = 0;
    scoped_refptr<SSHSession> session = (SSHSession*)args[0].resource.get();
    if (!session->SetUserName(username) || !(methods = session->GetAuthMethod())) {
        return -1;
    }
    if (SSH_AUTH_METHOD_NONE == methods) {
        return 0;
    }
    const char* name_ptr = NULL;
    if (username.size()) {
        name_ptr = username.c_str();
    }
    if (password.size() && (methods & SSH_AUTH_METHOD_PASSWORD)) {
        rc = ssh_userauth_password(session->mSession, name_ptr, password.c_str());
        if (rc == SSH_AUTH_SUCCESS) {
            return Value(0);
        }
    }
    if (password.size() && (methods & SSH_AUTH_METHOD_INTERACTIVE)) {
        while ((rc = ssh_userauth_kbdint(session->mSession, name_ptr, NULL)) == SSH_AUTH_INFO) {
            const char* s;
            int n, nprompt;
            char echoflag;
            int found_prompt = 0;
            nprompt = ssh_userauth_kbdint_getnprompts(session->mSession);
            for (n = 0; n < nprompt; n++) {
                s = ssh_userauth_kbdint_getprompt(session->mSession, n, &echoflag);
                if (s && *s && !echoflag && !found_prompt) {
                    found_prompt = 1;
                    rc = ssh_userauth_kbdint_setanswer(session->mSession, n, password.c_str());
                    if (rc == SSH_AUTH_SUCCESS) {
                        return Value(0);
                    }
                }
            }
        }
    }
    if (privateKey.size() && (methods & SSH_AUTH_METHOD_PUBLICKEY)) {
        ssh_key key = NULL;

        if (ssh_pki_import_privkey_base64(privateKey.c_str(), passphrase.c_str(), NULL, NULL,
                                          &key)) {
            return -1;
        }
        if ((rc = ssh_userauth_try_publickey(session->mSession, name_ptr, key)) !=
            SSH_AUTH_SUCCESS) {
            rc = ssh_userauth_publickey(session->mSession, name_ptr, key);
        }
        ssh_key_free(key);
        if (rc != SSH_AUTH_SUCCESS) {
            return -1;
        }
        return 0;
    }
    return -1;
}

//session,username,password
Value SSHLoginInteractive(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    std::string username = GetString(args, 1);
    std::string password = GetString(args, 2);
    int rc = 0;
    const char* s = NULL;
    scoped_refptr<SSHSession> session = (SSHSession*)args[0].resource.get();
    if (!session->SetUserName(username) || !session->GetAuthMethod()) {
        return Value(-1);
    }
    if (password.size()) {
        rc = ssh_userauth_kbdint_setanswer(session->mSession, 0, password.c_str());

        if (rc < 0) {
            return -1;
        }

        if (rc == 0) {
            /* I need to do that to finish the auth process. */
            while ((rc = ssh_userauth_kbdint(session->mSession, NULL, NULL)) == SSH_AUTH_INFO) {
                ssh_userauth_kbdint_getnprompts(session->mSession);
            }
            if (rc == SSH_AUTH_SUCCESS) {
                return 0;
            }
        }
        return -1;

    } else {
        while ((rc = ssh_userauth_kbdint(session->mSession, NULL, NULL)) == SSH_AUTH_INFO) {
            int n, nprompt;
            char echoflag;
            int found_prompt = 0;

            nprompt = ssh_userauth_kbdint_getnprompts(session->mSession);
            for (n = 0; n < nprompt; n++) {
                s = ssh_userauth_kbdint_getprompt(session->mSession, n, &echoflag);
                if (s && *s && !echoflag && !found_prompt) {
                    return Value(s);
                }
            }
        }
        return Value();
    }
}

int exec_ssh_cmd(ssh_session session, const std::string& cmd, std::string& out,
                 std::string& error) {
    int rc = 1;
    ssh_channel channel;
    char buffer[4096];
    if ((channel = ssh_channel_new(session)) == NULL) {
        LOG("ssh_channel_new failed ", ssh_get_error(session));
        return SSH_ERROR;
    }

    if (ssh_channel_open_session(channel)) {
        ssh_channel_free(channel);
        return SSH_ERROR;
    }
    ssh_channel_request_pty(channel);
    if (ssh_channel_request_exec(channel, cmd.c_str())) {
        ssh_channel_free(channel);
        return SSH_ERROR;
    }
    while (rc > 0) {
        if ((rc = ssh_channel_read_timeout(channel, buffer, sizeof(buffer), 1, 15000)) > 0) {
            out.append(buffer, rc);
        }
        if (rc == SSH_ERROR) goto exec_err;
    }
    rc = 1;
    while (rc > 0) {
        if ((rc = ssh_channel_read_timeout(channel, buffer, sizeof(buffer), 0, 15000)) > 0) {
            error.append(buffer, rc);
        }
        if (rc == SSH_ERROR) goto exec_err;
    }
    rc = SSH_OK;

exec_err:
    ssh_channel_free(channel);
    return rc;
}

//session,cmd
Value SSHExecute(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    std::string cmd = GetString(args, 1);
    std::string out, error;
    scoped_refptr<SSHSession> session = (SSHSession*)args[0].resource.get();
    int rc = exec_ssh_cmd(session->mSession, cmd, out, error);
    if (rc != SSH_OK) {
        return Value();
    }
    Value ret = Value::make_map();
    ret["Stdout"] = out;
    ret["StdErr"] = error;
    return ret;
}
//session,username
Value SSHGetIssueBanner(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    std::string username = GetString(args, 1);
    scoped_refptr<SSHSession> session = (SSHSession*)args[0].resource.get();
    if (!session->SetUserName(username) || !session->GetAuthMethod()) {
        LOG("invalid methods ", session->GetAuthMethod(), session->SetUserName(username));
        return "";
    }
    char* banner = ssh_get_issue_banner(session->mSession);
    if (banner == NULL) {
        return Value();
    }
    Value ret(banner);
    ssh_string_free_char(banner);
    return ret;
}

Value SSHGetServerBanner(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    scoped_refptr<SSHSession> session = (SSHSession*)args[0].resource.get();
    const char* banner = ssh_get_serverbanner(session->mSession);
    if (banner == NULL) {
        LOG(ssh_get_error(session->mSession));
        return Value();
    }
    Value ret(banner);
    return ret;
}

Value SSHGetPUBKey(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    scoped_refptr<SSHSession> session = (SSHSession*)args[0].resource.get();
    ssh_string key = ssh_get_pubkey(session->mSession);
    if (key == NULL) {
        LOG(ssh_get_error(session->mSession));
        return Value();
    }
    std::string strKey(ssh_string_to_char(key), ssh_string_len(key));
    ssh_string_free(key);
    return strKey;
}

//session,username
Value SSHGetAuthMethod(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    std::string username = GetString(args, 1);
    scoped_refptr<SSHSession> session = (SSHSession*)args[0].resource.get();
    if (!session->SetUserName(username) || !session->GetAuthMethod()) {
        LOG(ssh_get_error(session->mSession));
        return "";
    }
    int methods = session->GetAuthMethod();
    std::string str = "";
    if (methods & SSH_AUTH_METHOD_NONE) {
        if (str.size()) {
            str += ",none";
        } else {
            str += "none";
        }
    }
    if (methods & SSH_AUTH_METHOD_PASSWORD) {
        if (str.size()) {
            str += ",password";
        } else {
            str += "password";
        }
    }
    if (methods & SSH_AUTH_METHOD_PUBLICKEY) {
        if (str.size()) {
            str += ",publickey";
        } else {
            str += "publickey";
        }
    }
    if (methods & SSH_AUTH_METHOD_HOSTBASED) {
        if (str.size()) {
            str += ",hostbased";
        } else {
            str += "hostbased";
        }
    }
    if (methods & SSH_AUTH_METHOD_INTERACTIVE) {
        if (str.size()) {
            str += ",keyboard-interactive";
        } else {
            str += "keyboard-interactive";
        }
    }
    return str;
}

//session,username
Value SSHShellOpen(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    std::string username = GetString(args, 1);
    int pty = GetInt(args, 2, 1);
    scoped_refptr<SSHSession> session = (SSHSession*)args[0].resource.get();
    if (!session->SetUserName(username) || !session->GetAuthMethod()) {
        return -1;
    }
    scoped_refptr<SSHChannel> channel = new SSHChannel(session);
    if (!channel->Open() || !channel->Shell(pty)) {
        return Value();
    }
    return Value((Resource*)channel.get());
}

//channel
Value SSHShellRead(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    std::string name;
    if (!args[0].IsResource(name) || name != "SSH-Channel") {
        return Value();
    }
    name = "";
    scoped_refptr<SSHChannel> channel = (SSHChannel*)args[0].resource.get();
    if (channel->readNB(name)) {
        return Value();
    }
    return name;
}

//channel
Value SSHShellWrite(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_STRING(1);
    std::string name;
    if (!args[0].IsResource(name) || name != "SSH-Channel") {
        return Value();
    }
    scoped_refptr<SSHChannel> channel = (SSHChannel*)args[0].resource.get();
    return channel->write(args[1].bytes);
}
