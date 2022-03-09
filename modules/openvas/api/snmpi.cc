extern "C"{
    #include <net-snmp/net-snmp-config.h>
    #include <net-snmp/net-snmp-includes.h>
}
#include <assert.h>
/*
 * @brief SNMP Get query value.
 *
 * param[in]    session     SNMP session.
 * param[in]    oid_str     OID string.
 * param[out]   result      Result of query.
 *
 * @return 0 if success and result value, -1 otherwise.
 */
int snmp_get(struct snmp_session* session, const char* oid_str, char** result) {
    struct snmp_session* ss;
    struct snmp_pdu *query, *response;
    oid oid_buf[MAX_OID_LEN];
    size_t oid_size = MAX_OID_LEN;
    int status;

    ss = snmp_open(session);
    if (!ss) {
        snmp_error(session, &status, &status, result);
        return -1;
    }
    query = snmp_pdu_create(SNMP_MSG_GET);
    read_objid(oid_str, oid_buf, &oid_size);
    snmp_add_null_var(query, oid_buf, oid_size);
    status = snmp_synch_response(ss, query, &response);
    if (status != STAT_SUCCESS) {
        snmp_error(ss, &status, &status, result);
        snmp_close(ss);
        return -1;
    }
    snmp_close(ss);

    if (response->errstat == SNMP_ERR_NOERROR) {
        struct variable_list* vars = response->variables;
        size_t res_len = 0, buf_len = 0;

        netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);
        sprint_realloc_value((u_char**)result, &buf_len, &res_len, 1, vars->name, vars->name_length,
                             vars);
        snmp_free_pdu(response);
        return 0;
    }
    #ifdef _WIN32
    *result = _strdup(snmp_errstring(response->errstat));
    #else
    *result = strdup(snmp_errstring(response->errstat));
    #endif
    snmp_free_pdu(response);
    return -1;
}

/*
 * @brief SNMPv3 Get query value.
 *
 * param[in]    peername    Target host in [protocol:]address[:port] format.
 * param[in]    username    Username value.
 * param[in]    authpass    Authentication password.
 * param[in]    authproto   Authentication protocol. 0 for md5, 1 for sha1.
 * param[in]    privpass    Privacy password.
 * param[in]    privproto   Privacy protocol. 0 for des, 1 for aes.
 * param[in]    oid_str     OID of value to get.
 * param[out]   result      Result of query.
 *
 * @return 0 if success and result value, -1 otherwise.
 */
int snmpv3_get(const char* peername, const char* username, const char* authpass, int authproto,
               const char* privpass, int privproto, const char* oid_str, char** result) {
    struct snmp_session session;

    assert(peername);
    assert(username);
    assert(authpass);
    assert(authproto == 0 || authproto == 1);
    assert(oid_str);
    assert(result);

    setenv("MIBS", "", 1);
    init_snmp("openvas");
    snmp_sess_init(&session);
    session.version = SNMP_VERSION_3;
    session.peername = (char*)peername;
    session.securityName = (char*)username;
    session.securityNameLen = strlen(session.securityName);

    if (privpass)
        session.securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;
    else
        session.securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;
    if (authproto == 0) {
        session.securityAuthProto = usmHMACMD5AuthProtocol;
        session.securityAuthProtoLen = USM_AUTH_PROTO_MD5_LEN;
    } else {
        session.securityAuthProto = usmHMACSHA1AuthProtocol;
        session.securityAuthProtoLen = USM_AUTH_PROTO_SHA_LEN;
    }
    session.securityAuthKeyLen = USM_AUTH_KU_LEN;
    if (generate_Ku(session.securityAuthProto, (u_int)session.securityAuthProtoLen, (u_char*)authpass,
                    strlen(authpass), session.securityAuthKey,
                    &session.securityAuthKeyLen) != SNMPERR_SUCCESS) {
        #ifdef _WIN32
        *result = _strdup("generate_Ku: Error");
        #else
        *result = strdup("generate_Ku: Error");
        #endif
        return -1;
    }
    if (privpass) {
        if (privproto) {
            session.securityPrivProto = usmAESPrivProtocol;
            session.securityPrivProtoLen = USM_PRIV_PROTO_AES_LEN;
        } else {
            session.securityPrivProto = usmDESPrivProtocol;
            session.securityPrivProtoLen = USM_PRIV_PROTO_DES_LEN;
        }
        session.securityPrivKeyLen = USM_PRIV_KU_LEN;
        if (generate_Ku(session.securityAuthProto, (u_int)session.securityAuthProtoLen,
                        (unsigned char*)privpass, strlen(privpass), session.securityPrivKey,
                        &session.securityPrivKeyLen) != SNMPERR_SUCCESS) {
#ifdef _WIN32
            *result = _strdup("generate_Ku: Error");
#else
            *result = strdup("generate_Ku: Error");
#endif
            return -1;
        }
    }

    return snmp_get(&session, oid_str, result);
}

/*
 * @brief SNMP v1 or v2c Get query value.
 *
 * param[in]    peername    Target host in [protocol:]address[:port] format.
 * param[in]    community   SNMP community string.
 * param[in]    oid_str     OID string of value to get.
 * param[in]    version     SNMP_VERSION_1 or SNMP_VERSION_2c.
 * param[out]   result      Result of query.
 *
 * @return 0 if success and result value, -1 otherwise.
 */
int snmpv1v2c_get(const char* peername, const char* community, const char* oid_str, int version,
                  char** result) {
    struct snmp_session session;

    assert(peername);
    assert(community);
    assert(oid_str);
    assert(version == SNMP_VERSION_1 || version == SNMP_VERSION_2c);

    setenv("MIBS", "", 1);
    snmp_sess_init(&session);
    session.version = version;
    session.peername = (char*)peername;
    session.community = (u_char*)community;
    session.community_len = strlen(community);

    return snmp_get(&session, oid_str, result);
}

