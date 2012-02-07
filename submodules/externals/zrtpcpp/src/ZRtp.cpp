/*
  Copyright (C) 2006-2009 Werner Dittmann

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*F
 * Authors: Werner Dittmann <Werner.Dittmann@t-online.de>
 */
#include <sstream>

#include <libzrtpcpp/crypto/ZrtpDH.h>
#include <libzrtpcpp/crypto/hmac256.h>
#include <libzrtpcpp/crypto/sha256.h>
#include <libzrtpcpp/crypto/hmac384.h>
#include <libzrtpcpp/crypto/sha384.h>
#include <libzrtpcpp/crypto/aesCFB.h>
#include <libzrtpcpp/crypto/twoCFB.h>

#include <libzrtpcpp/ZRtp.h>
#include <libzrtpcpp/ZrtpStateClass.h>
#include <libzrtpcpp/ZIDFile.h>
#include <libzrtpcpp/ZIDRecord.h>
#include <libzrtpcpp/Base32.h>

using namespace GnuZrtpCodes;

/* disabled...but used in testing and debugging, probably should have a
   controlling #define...
   *
static void hexdump(const char* title, const unsigned char *s, int l) {
    int n=0;

    if (s == NULL) return;

    fprintf(stderr, "%s",title);
    for( ; n < l ; ++n)
    {
        if((n%16) == 0)
            fprintf(stderr, "\n%04x",n);
        fprintf(stderr, " %02x",s[n]);
    }
    fprintf(stderr, "\n");
}
 */

/*
 * This method simplifies detection of libzrtpcpp inside Automake, configure
 * and friends
 */
#ifdef __cplusplus
extern "C" {
#endif
int ZrtpAvailable()
{
    return 1;
}
#ifdef __cplusplus
}
#endif

ZRtp::ZRtp(uint8_t *myZid, ZrtpCallback *cb, std::string id, 
    ZrtpConfigure* config, bool mitmm):
    callback(cb), dhContext(NULL), DHss(NULL), auxSecret(NULL), 
    auxSecretLength(0), rs1Valid(false), rs2Valid(false), msgShaContext(NULL),
    multiStream(false), multiStreamAvailable(false), pbxSecretTmp(NULL),
    configureAlgos(*config) {

    enableMitmEnrollment = config->isTrustedMitM();

   // setup the implicit hash function pointers and length
    hashLengthImpl = SHA256_DIGEST_LENGTH;
    hashFunctionImpl = sha256;
    hashListFunctionImpl = sha256;

    hmacFunctionImpl = hmac_sha256;
    hmacListFunctionImpl = hmac_sha256;

    /*
     * Generate H0 as a random number (256 bits, 32 bytes) and then
     * the hash chain, refer to chapter 10. Use the implicit hash function.
     */
    randomZRTP(H0, HASH_IMAGE_SIZE);
    sha256(H0, HASH_IMAGE_SIZE, H1);        // hash H0 and generate H1
    sha256(H1, HASH_IMAGE_SIZE, H2);        // H2
    sha256(H2, HASH_IMAGE_SIZE, H3);        // H3

    zrtpHello.configureHello(&configureAlgos);
    zrtpHello.setH3(H3);             // set H3 in Hello, included in helloHash

    memcpy(zid, myZid, ZID_SIZE);
    zrtpHello.setZid(zid);
    if (mitmm)                   // this session acts for a trusted MitM (PBX)
        zrtpHello.setMitmMode();
    setClientId(id);                // set id, compute HMAC and final helloHash

    stateEngine = new ZrtpStateClass(this);
}

ZRtp::~ZRtp() {
    stopZrtp();
    if (DHss != NULL) {
	delete DHss;
        DHss = NULL;
    }
    if (stateEngine != NULL) {
	delete stateEngine;
        stateEngine = NULL;
    }
    if (dhContext != NULL) {
	delete dhContext;
        dhContext = NULL;
    }
    if (msgShaContext != NULL) {
        closeHashCtx(msgShaContext, NULL);
        msgShaContext = NULL;
    }
    if (auxSecret != NULL) {
        delete auxSecret;
        auxSecret = NULL;
        auxSecretLength = 0;
    }
    memset(hmacKeyI, 0, MAX_DIGEST_LENGTH);
    memset(hmacKeyR, 0, MAX_DIGEST_LENGTH);

    memset(zrtpKeyI, 0, MAX_DIGEST_LENGTH);
    memset(zrtpKeyR, 0, MAX_DIGEST_LENGTH);
    /*
     * Clear the Initiator's srtp key and salt
     */
    memset(srtpKeyI, 0, MAX_DIGEST_LENGTH);
    memset(srtpSaltI, 0,  MAX_DIGEST_LENGTH);
    /*
     * Clear he Responder's srtp key and salt
     */
    memset(srtpKeyR, 0, MAX_DIGEST_LENGTH);
    memset(srtpSaltR, 0, MAX_DIGEST_LENGTH);

    memset(zrtpSession, 0, MAX_DIGEST_LENGTH);
}

void ZRtp::processZrtpMessage(uint8_t *message, uint32_t pSSRC) {
    Event_t ev;

    peerSSRC = pSSRC;
    ev.type = ZrtpPacket;
    ev.packet = message;

    if (stateEngine != NULL) {
        stateEngine->processEvent(&ev);
    }
}

void ZRtp::processTimeout() {
    Event_t ev;

    ev.type = Timer;
    if (stateEngine != NULL) {
        stateEngine->processEvent(&ev);
    }
}

#ifdef oldgoclear
bool ZRtp::handleGoClear(uint8_t *message)
{
    char *msg, first, last;

    msg = (char *)message + 4;
    first = tolower(*msg);
    last = tolower(*(msg+6));

    if (first == 'g' && last == 'r') {
        Event_t ev;

        ev.type = ZrtpGoClear;
        ev.packet = message;
        if (stateEngine != NULL) {
            stateEngine->processEvent(&ev);
        }
        return true;
    }
    else {
        return false;
    }
}
#endif

void ZRtp::startZrtpEngine() {
    Event_t ev;

    ev.type = ZrtpInitial;
    stateEngine->processEvent(&ev);
}

void ZRtp::stopZrtp() {
    Event_t ev;

    if (stateEngine != NULL) {
        ev.type = ZrtpClose;
        stateEngine->processEvent(&ev);
    }
}

bool ZRtp::inState(int32_t state)
{
    if (stateEngine != NULL) {
        return stateEngine->inState(state);
    }
    else {
        return false;
    }
}

ZrtpPacketHello* ZRtp::prepareHello() {
    return &zrtpHello; 
}

ZrtpPacketHelloAck* ZRtp::prepareHelloAck() {
    return &zrtpHelloAck;
}

/*
 * At this point we will assume the role of Initiator. This role may change
 * in case we have a commit-clash. Refer to chapter 5.2 in the spec how
 * to break this tie.
 */
ZrtpPacketCommit* ZRtp::prepareCommit(ZrtpPacketHello *hello, uint32_t* errMsg) {

    sendInfo(Info, InfoHelloReceived);

    if (memcmp(hello->getVersion(), zrtpVersion, ZRTP_WORD_SIZE-1) != 0) {
        *errMsg = UnsuppZRTPVersion;
        return NULL;
    }
    // Save our peer's (presumably the Responder) ZRTP id
    memcpy(peerZid, hello->getZid(), ZID_SIZE);
    if (memcmp(peerZid, zid, ZID_SIZE) == 0) {       // peers have same ZID????
        *errMsg = EqualZIDHello;
        return NULL;
    }
    memcpy(peerH3, hello->getH3(), HASH_IMAGE_SIZE);

    /*
     * The Following section extracts the algorithm from the peer's Hello
     * packet. Always the preferend offered algorithms are
     * used. If the received Hello does not contain algo specifiers
     * or offers only unsupported optional algos then replace
     * these with mandatory algos and put them into the Commit packet.
     * Refer to the findBest*() functions.
     * If this is a MultiStream ZRTP object then do not get the cipher,
     * authentication from hello packet but use the pre-initialized values
     * as proposed by the standard. If we switch to responder mode the
     * commit packet may contain other algos - see function
     * prepareConfirm2MultiStream(...).
     */
    sasType = findBestSASType(hello);

    if (!multiStream) {
        authLength = findBestAuthLen(hello);
        pubKey = findBestPubkey(hello);
        cipher = findBestCipher(hello, pubKey);
        hash = findBestHash(hello);
        multiStreamAvailable = checkMultiStream(hello);
    }
    else {
        if (checkMultiStream(hello)) {
            return prepareCommitMultiStream(hello);
        }
        else {
            // we are in multi-stream but peer does not offer multi-stream
            // return error code to other party - unsupported PK, must be Mult
            *errMsg = UnsuppPKExchange;
            return NULL;
        }
    }
    setNegotiatedHash(hash);

    // Modify here when introducing new DH key agreement, for example
    // elliptic curves.
    dhContext = new ZrtpDH(pubKey->getName());
    dhContext->generatePublicKey();

//    pubKeyLen = dhContext->getPubKeySize();
    dhContext->getPubKeyBytes(pubKeyBytes);
    sendInfo(Info, InfoCommitDHGenerated);

    // Prepare IV data that we will use during confirm packet encryption. 
    randomZRTP(randomIV, sizeof(randomIV));

    /*
     * Prepare our DHPart2 packet here. Required to compute HVI. If we stay
     * in Initiator role then we reuse this packet later in prepareDHPart2().
     * To create this DH packet we have to compute the retained secret ids
     * first. Thus get our peer's retained secret data first.
     */
    ZIDRecord zidRec(peerZid);
    ZIDFile *zidFile = ZIDFile::getInstance();
    zidFile->getRecord(&zidRec);

    //Compute the Initator's and Responder's retained secret ids.
    computeSharedSecretSet(zidRec);

    // Check for PBX and if we have a MitM Key available. This would
    // qualify the PBX as trusted MitM and that this client enrolled to
    // the trusted PBX
    if (hello->isMitmMode()) {
        mitmSeen = true;
        trustedMitM = zidRec.isMITMKeyAvailable();
    }
    // Construct a DHPart2 message (Initiator's DH message). This packet
    // is required to compute the HVI (Hash Value Initiator), refer to 
    // chapter 5.4.1.1.

    // Fill the values in the DHPart2 packet
    zrtpDH2.setPubKeyType(pubKey->getName());
    zrtpDH2.setMessageType((uint8_t*)DHPart2Msg);
    zrtpDH2.setRs1Id(rs1IDi);
    zrtpDH2.setRs2Id(rs2IDi);
    zrtpDH2.setAuxSecretId(auxSecretIDi);
    zrtpDH2.setPbxSecretId(pbxSecretIDi);
    zrtpDH2.setPv(pubKeyBytes);
    zrtpDH2.setH1(H1);

    int32_t len = zrtpDH2.getLength() * ZRTP_WORD_SIZE;

    // Compute HMAC over DH2, excluding the HMAC field (HMAC_SIZE)
    // and store in DH2. Key to HMAC is H0, use HASH_IMAGE_SIZE bytes only.
    // Must use implicit HMAC functions.
    uint8_t hmac[IMPL_MAX_DIGEST_LENGTH];
    uint32_t macLen;
    hmacFunctionImpl(H0, HASH_IMAGE_SIZE, (uint8_t*)zrtpDH2.getHeaderBase(), 
                len-(HMAC_SIZE), hmac, &macLen);
    zrtpDH2.setHMAC(hmac);

    // Compute the HVI, refer to chapter 5.4.1.1 of the specification
    computeHvi(&zrtpDH2, hello);

    zrtpCommit.setZid(zid);
    zrtpCommit.setHashType((uint8_t*)hash->getName());
    zrtpCommit.setCipherType((uint8_t*)cipher->getName());
    zrtpCommit.setAuthLen((uint8_t*)authLength->getName());
    zrtpCommit.setPubKeyType((uint8_t*)pubKey->getName());
    zrtpCommit.setSasType((uint8_t*)sasType->getName());
    zrtpCommit.setHvi(hvi);
    zrtpCommit.setH2(H2);

    len = zrtpCommit.getLength() * ZRTP_WORD_SIZE;

    // Compute HMAC over Commit, excluding the HMAC field (HMAC_SIZE)
    // and store in Hello. Key to HMAC is H1, use HASH_IMAGE_SIZE bytes only.
    // Must use implicit HMAC functions.
    hmacFunctionImpl(H1, HASH_IMAGE_SIZE, (uint8_t*)zrtpCommit.getHeaderBase(),
                len-(HMAC_SIZE), hmac, &macLen);
    zrtpCommit.setHMAC(hmac);

    // hash first messages to produce overall message hash
    // First the Responder's Hello message, second the Commit 
    // (always Initator's). 
    // Must use negotiated hash.
    msgShaContext = createHashCtx();
    hashCtxFunction(msgShaContext, (unsigned char*)hello->getHeaderBase(), hello->getLength() * ZRTP_WORD_SIZE);
    hashCtxFunction(msgShaContext, (unsigned char*)zrtpCommit.getHeaderBase(), len);

    // store Hello data temporarily until we can check HMAC after receiving Commit as
    // Responder or DHPart1 as Initiator 
    storeMsgTemp(hello);
    return &zrtpCommit;
}

ZrtpPacketCommit* ZRtp::prepareCommitMultiStream(ZrtpPacketHello *hello) {

    randomZRTP(hvi, ZRTP_WORD_SIZE*4);  // This is the Multi-Stream NONCE size

    zrtpCommit.setZid(zid);
    zrtpCommit.setHashType((uint8_t*)hash->getName());
    zrtpCommit.setCipherType((uint8_t*)cipher->getName());
    zrtpCommit.setAuthLen((uint8_t*)authLength->getName());
    zrtpCommit.setPubKeyType((uint8_t*)"Mult");  // this is fixed because of Multi Stream mode
    zrtpCommit.setSasType((uint8_t*)sasType->getName());
    zrtpCommit.setNonce(hvi);
    zrtpCommit.setH2(H2);

    int32_t len = zrtpCommit.getLength() * ZRTP_WORD_SIZE;

    // Compute HMAC over Commit, excluding the HMAC field (HMAC_SIZE)
    // and store in Hello. Key to HMAC is H1, use HASH_IMAGE_SIZE bytes only.
    // Must use the implicit HMAC function.
    uint8_t hmac[IMPL_MAX_DIGEST_LENGTH];
    uint32_t macLen;
    hmacFunctionImpl(H1, HASH_IMAGE_SIZE, (uint8_t*)zrtpCommit.getHeaderBase(), 
                len-(HMAC_SIZE), hmac, &macLen);
    zrtpCommit.setHMACMulti(hmac);


    // hash first messages to produce overall message hash
    // First the Responder's Hello message, second the Commit 
    // (always Initator's). 
    // Must use the negotiated hash.
    msgShaContext = createHashCtx();

    hashCtxFunction(msgShaContext, (unsigned char*)hello->getHeaderBase(), hello->getLength() * ZRTP_WORD_SIZE);
    hashCtxFunction(msgShaContext, (unsigned char*)zrtpCommit.getHeaderBase(), len);

    // store Hello data temporarily until we can check HMAC after receiving Commit as
    // Responder or DHPart1 as Initiator 
    storeMsgTemp(hello);
    return &zrtpCommit;
}

/*
 * At this point we will take the role of the Responder. We may have been in 
 * the role of the Initiator before and already sent a commit packet that
 * clashed with a commit packet from our peer. If our HVI was lower than our
 * peer's HVI then we switched to Responder and handle our peer's commit packet
 * here. This method takes care to delete and refresh data left over from a
 * possible Initiator preparation. This belongs to prepared DH data, message 
 * hash SHA context
 */
ZrtpPacketDHPart* ZRtp::prepareDHPart1(ZrtpPacketCommit *commit, uint32_t* errMsg) {

    sendInfo(Info, InfoRespCommitReceived);

    // The following code check the hash chain according chapter 10 to detect
    // false ZRTP packets.
    // Must use the implicit hash function.
    uint8_t tmpH3[IMPL_MAX_DIGEST_LENGTH];
    memcpy(peerH2, commit->getH2(), HASH_IMAGE_SIZE);
    hashFunctionImpl(peerH2, HASH_IMAGE_SIZE, tmpH3);

    if (memcmp(tmpH3, peerH3, HASH_IMAGE_SIZE) != 0) {
        *errMsg = IgnorePacket;
        return NULL;
    }

    // Check HMAC of previous Hello packet stored in temporary buffer. The
    // HMAC key of peer's Hello packet is peer's H2 that is contained in the 
    // Commit packet. Refer to chapter 9.1.
    if (!checkMsgHmac(peerH2)) {
        sendInfo(Severe, SevereHelloHMACFailed);
        *errMsg = CriticalSWError;
        return NULL;
    }

    // check if we support the commited Cipher type
    AlgorithmEnum* cp = &zrtpSymCiphers.getByName((const char*)commit->getCipherType());
    if (!cp->isValid()) { // no match - something went wrong
        *errMsg = UnsuppCiphertype;
        return NULL;
    }
    cipher = cp;

    // check if we support the commited Authentication length
    cp = &zrtpAuthLengths.getByName((const char*)commit->getAuthLen());
    if (!cp->isValid()) { // no match - something went wrong
        *errMsg = UnsuppSRTPAuthTag;
        return NULL;
    }
    authLength = cp;

    // check if we support the commited hash type
    cp = &zrtpHashes.getByName((const char*)commit->getHashType());
    if (!cp->isValid()) { // no match - something went wrong
        *errMsg = UnsuppHashType;
        return NULL;
    }
    // check if the peer's commited hash is the same that we used when
    // preparing our commit packet. If not do the necessary resets and
    // recompute some data.
    if (*(int32_t*)(hash->getName()) != *(int32_t*)(cp->getName())) {
        hash = cp;
        setNegotiatedHash(hash);

        ZIDRecord zidRec(peerZid);
        ZIDFile *zidFile = ZIDFile::getInstance();
        zidFile->getRecord(&zidRec);

        // Compute the Initator's and Responder's retained secret ids
        // with the committed hash.
        computeSharedSecretSet(zidRec);
    }

    // check if we support the commited pub key type
    cp = &zrtpPubKeys.getByName((const char*)commit->getPubKeysType());
    if (!cp->isValid()) { // no match - something went wrong
        *errMsg = UnsuppPKExchange;
        return NULL;
    }
    pubKey = cp;

    // check if we support the commited SAS type
    cp = &zrtpSasTypes.getByName((const char*)commit->getSasType());
    if (!cp->isValid()) { // no match - something went wrong
        *errMsg = UnsuppSASScheme;
        return NULL;
    }
    sasType = cp;

    // dhContext cannot be NULL - always setup during prepareCommit()
    // check if we can use the dhContext prepared by prepareCOmmit(),
    // if not delete old DH context and generate new one
    // The algorithm names are 4 chars only, thus we can cast to int32_t
    if (*(int32_t*)(dhContext->getDHtype()) != *(int32_t*)(pubKey->getName())) {
        delete dhContext;
        dhContext = new ZrtpDH(pubKey->getName());
        dhContext->generatePublicKey();
    }
    sendInfo(Info, InfoDH1DHGenerated);

    dhContext->getPubKeyBytes(pubKeyBytes);

    // Setup a DHPart1 packet.
    zrtpDH1.setPubKeyType(pubKey->getName());
    zrtpDH1.setMessageType((uint8_t*)DHPart1Msg);
    zrtpDH1.setRs1Id(rs1IDr);
    zrtpDH1.setRs2Id(rs2IDr);
    zrtpDH1.setAuxSecretId(auxSecretIDr);
    zrtpDH1.setPbxSecretId(pbxSecretIDr);
    zrtpDH1.setPv(pubKeyBytes);
    zrtpDH1.setH1(H1);

    int32_t len = zrtpDH1.getLength() * ZRTP_WORD_SIZE;

    // Compute HMAC over DHPart1, excluding the HMAC field (HMAC_SIZE)
    // and store in DHPart1.
    // Use implicit Hash function
    uint8_t hmac[IMPL_MAX_DIGEST_LENGTH];
    uint32_t macLen;
    hmacFunctionImpl(H0, HASH_IMAGE_SIZE, (uint8_t*)zrtpDH1.getHeaderBase(), 
                len-(HMAC_SIZE), hmac, &macLen);
    zrtpDH1.setHMAC(hmac);

    // We are definitly responder. Save the peer's hvi for later compare.
    myRole = Responder;
    memcpy(peerHvi, commit->getHvi(), HVI_SIZE);

    // We are responder. Release a possibly pre-computed SHA256 context
    // because this was prepared for Initiator. Then create a new one.
    if (msgShaContext != NULL) {
        closeHashCtx(msgShaContext, NULL);
    }
    msgShaContext = createHashCtx();

    // Hash messages to produce overall message hash:
    // First the Responder's (my) Hello message, second the Commit 
    // (always Initator's), then the DH1 message (which is always a 
    // Responder's message).
    // Must use negotiated hash
    hashCtxFunction(msgShaContext, (unsigned char*)zrtpHello.getHeaderBase(),
              zrtpHello.getLength() * ZRTP_WORD_SIZE);
    hashCtxFunction(msgShaContext, (unsigned char*)commit->getHeaderBase(),
              commit->getLength() * ZRTP_WORD_SIZE);
    hashCtxFunction(msgShaContext, (unsigned char*)zrtpDH1.getHeaderBase(),
              zrtpDH1.getLength() * ZRTP_WORD_SIZE);

    // store Commit data temporarily until we can check HMAC after we got DHPart2
    storeMsgTemp(commit);

    return &zrtpDH1;
}

/*
 * At this point we will take the role of the Initiator.
 */
ZrtpPacketDHPart* ZRtp::prepareDHPart2(ZrtpPacketDHPart *dhPart1, uint32_t* errMsg) {

    uint8_t* pvr;

    sendInfo(Info, InfoInitDH1Received);

    // Because we are initiator the protocol engine didn't receive Commit
    // thus could not store a peer's H2. A two step SHA256 is required to 
    // re-compute H3. Then compare with peer's H3 from peer's Hello packet.
    // Must use implicit hash function.
    uint8_t tmpHash[IMPL_MAX_DIGEST_LENGTH];
    hashFunctionImpl(dhPart1->getH1(), HASH_IMAGE_SIZE, tmpHash); // Compute peer's H2
    memcpy(peerH2, tmpHash, HASH_IMAGE_SIZE);
    hashFunctionImpl(peerH2, HASH_IMAGE_SIZE, tmpHash);          // Compute peer's H3 (tmpHash)

    if (memcmp(tmpHash, peerH3, HASH_IMAGE_SIZE) != 0) {
        *errMsg = IgnorePacket;
        return NULL;
    }

    // Check HMAC of previous Hello packet stored in temporary buffer. The
    // HMAC key of the Hello packet is peer's H2 that was computed above.
    // Refer to chapter 9.1 and chapter 10.
    if (!checkMsgHmac(peerH2)) {
        sendInfo(Severe, SevereHelloHMACFailed);
        *errMsg = CriticalSWError;
        return NULL;
    }

    // get memory to store DH result TODO: make it fixed memory
    DHss = new uint8_t[dhContext->getDhSize()];
    if (DHss == NULL) {
        *errMsg = CriticalSWError;
        return NULL;
    }

    // get and check Responder's public value, see chap. 5.4.3 in the spec
    pvr = dhPart1->getPv();
    if (!dhContext->checkPubKey(pvr)) {
        *errMsg = DHErrorWrongPV;
        return NULL;
    }
    dhContext->computeSecretKey(pvr, DHss);

    myRole = Initiator;

    // We are Initiator: the Responder's Hello and the Initiator's (our) Commit
    // are already hashed in the context. Now hash the Responder's DH1 and then
    // the Initiator's (our) DH2 in that order.
    // Use the negotiated hash function.
    hashCtxFunction(msgShaContext, (unsigned char*)dhPart1->getHeaderBase(), 
		    dhPart1->getLength() * ZRTP_WORD_SIZE);
    hashCtxFunction(msgShaContext, (unsigned char*)zrtpDH2.getHeaderBase(), 
		    zrtpDH2.getLength() * ZRTP_WORD_SIZE);

    // Compute the message Hash
    closeHashCtx(msgShaContext, messageHash);
    msgShaContext = NULL;

    // To compute the keys for the Initiator we need the retained secrets of our
    // peer. Get them from the storage.
    ZIDRecord zidRec(peerZid);
    ZIDFile *zid = ZIDFile::getInstance();
    zid->getRecord(&zidRec);

    // Now compute the S0, all dependend keys and the new RS1
    generateKeysInitiator(dhPart1, zidRec);
    zid->saveRecord(&zidRec);

    delete dhContext;
    dhContext = NULL;

    // TODO: at initiator we can call signSAS at this point, don't dealy until confirm1 reveived
    // store DHPart1 data temporarily until we can check HMAC after receiving Confirm1
    storeMsgTemp(dhPart1);
    return &zrtpDH2;
}

/*
 * At this point we are Responder.
 */
ZrtpPacketConfirm* ZRtp::prepareConfirm1(ZrtpPacketDHPart* dhPart2, uint32_t* errMsg) {

    uint8_t* pvi;

    sendInfo(Info, InfoRespDH2Received);

    // Because we are responder we received a Commit and stored its H2.
    // Now re-compute H2 from received H1 and compare with stored peer's H2.
    // Use implicit hash function
    uint8_t tmpHash[IMPL_MAX_DIGEST_LENGTH];
    hashFunctionImpl(dhPart2->getH1(), HASH_IMAGE_SIZE, tmpHash);
    if (memcmp(tmpHash, peerH2, HASH_IMAGE_SIZE) != 0) {
        *errMsg = IgnorePacket;
        return NULL;
    }

    // Check HMAC of Commit packet stored in temporary buffer. The
    // HMAC key of the Commit packet is peer's H1 that is contained in
    // DHPart2. Refer to chapter 9.1 and chapter 10.
    if (!checkMsgHmac(dhPart2->getH1())) {
        sendInfo(Severe, SevereCommitHMACFailed);
        *errMsg = CriticalSWError;
        return NULL;
    }
    // Now we have the peer's pvi. Because we are responder re-compute my hvi
    // using my Hello packet and the Initiator's DHPart2 and compare with
    // hvi sent in commit packet. If it doesn't macht then a MitM attack
    // may have occured.
    computeHvi(dhPart2, &zrtpHello);
    if (memcmp(hvi, peerHvi, HVI_SIZE) != 0) {
        *errMsg = DHErrorWrongHVI;
        return NULL;
    }
    DHss = new uint8_t[dhContext->getDhSize()];
    if (DHss == NULL) {
        *errMsg = CriticalSWError;
        return NULL;
    }
    // Get and check the Initiator's public value, see chap. 5.4.2 of the spec
    pvi = dhPart2->getPv();
    if (!dhContext->checkPubKey(pvi)) {
        *errMsg = DHErrorWrongPV;
        return NULL;
    }
    dhContext->computeSecretKey(pvi, DHss);
    // Hash the Initiator's DH2 into the message Hash (other messages already
    // prepared, see method prepareDHPart1().
    // Use neotiated hash function
    hashCtxFunction(msgShaContext, (unsigned char*)dhPart2->getHeaderBase(),
              dhPart2->getLength() * ZRTP_WORD_SIZE);

    closeHashCtx(msgShaContext, messageHash);
    msgShaContext = NULL;

    // To compute the Keys for the Initiator we need the retained secrets of our
    // peer. Get them from the storage.
    ZIDRecord zidRec(peerZid);
    ZIDFile *zid = ZIDFile::getInstance();
    zid->getRecord(&zidRec);

    /*
     * The expected shared secret Ids were already computed when we built the
     * DHPart1 packet. Generate s0, all depended keys, and the new RS1 value
     * for the ZID record.
     */
    generateKeysResponder(dhPart2, zidRec);
    zid->saveRecord(&zidRec);

    delete dhContext;
    dhContext = NULL;

    // Fill in Confirm1 packet.
    zrtpConfirm1.setMessageType((uint8_t*)Confirm1Msg);
    // TODO: at this point sas hash is ready, perform signSAS callback and fill in signature if available
    zrtpConfirm1.setSignatureLength(0);

    // Check if user verfied the SAS in a previous call and thus verfied
    // the retained secret.
    if (zidRec.isSasVerified()) {
        zrtpConfirm1.setSASFlag();
    }
    zrtpConfirm1.setExpTime(0xFFFFFFFF);
    zrtpConfirm1.setIv(randomIV);
    zrtpConfirm1.setHashH0(H0);

    // if this run at PBX user agent enrollment service then set flag in confirm
    // packet and store the MitM key
    if (enrollmentMode) {
        computePBXSecret();
        zrtpConfirm1.setPBXEnrollment();
        writeEnrollmentPBX();
    }
    uint8_t confMac[MAX_DIGEST_LENGTH];
    uint32_t macLen;

    // Encrypt and HMAC with Responder's key - we are Respondere here
    int hmlen = (zrtpConfirm1.getLength() - 9) * ZRTP_WORD_SIZE;
	cipher->getEncrypt()(zrtpKeyR, cipher->getKeylen(), randomIV,
                         zrtpConfirm1.getHashH0(), hmlen);
    hmacFunction(hmacKeyR, hashLength,
                (unsigned char*)zrtpConfirm1.getHashH0(),
                hmlen, confMac, &macLen);

    zrtpConfirm1.setHmac(confMac);

   // store DHPart2 data temporarily until we can check HMAC after receiving Confirm2
    storeMsgTemp(dhPart2);
    return &zrtpConfirm1;
}

/*
 * At this point we are Responder.
 */
ZrtpPacketConfirm* ZRtp::prepareConfirm1MultiStream(ZrtpPacketCommit* commit, uint32_t* errMsg) {

    sendInfo(Info, InfoRespCommitReceived);

    // The following code checks the hash chain according chapter 10 to detect
    // false ZRTP packets.
    // Use implicit hash function
    uint8_t tmpH3[IMPL_MAX_DIGEST_LENGTH];
    memcpy(peerH2, commit->getH2(), HASH_IMAGE_SIZE);
    hashFunctionImpl(peerH2, HASH_IMAGE_SIZE, tmpH3);

    if (memcmp(tmpH3, peerH3, HASH_IMAGE_SIZE) != 0) {
        *errMsg = IgnorePacket;
        return NULL;
    }

    // Check HMAC of previous Hello packet stored in temporary buffer. The
    // HMAC key of peer's Hello packet is peer's H2 that is contained in the 
    // Commit packet. Refer to chapter 9.1.
    if (!checkMsgHmac(peerH2)) {
        sendInfo(Severe, SevereHelloHMACFailed);
        *errMsg = CriticalSWError;
        return NULL;
    }

    // check if Commit contains "Mult" as pub key type
    AlgorithmEnum* cp = &zrtpPubKeys.getByName((const char*)commit->getPubKeysType());
    if (!cp->isValid() || *(int32_t*)(cp->getName()) != *(int32_t*)mult) {
        *errMsg = UnsuppPKExchange;
        return NULL;
    }

    // check if we support the commited cipher
    cp = &zrtpSymCiphers.getByName((const char*)commit->getCipherType());
    if (!cp->isValid()) { // no match - something went wrong
        *errMsg = UnsuppCiphertype;
	return NULL;
    }
    cipher = cp;

    // check if we support the commited Authentication length
    cp = &zrtpAuthLengths.getByName((const char*)commit->getAuthLen());
    if (!cp->isValid()) { // no match - something went wrong
        *errMsg = UnsuppSRTPAuthTag;
        return NULL;
    }
    authLength = cp;

    // check if we support the commited hash type
    cp = &zrtpHashes.getByName((const char*)commit->getHashType());
    if (!cp->isValid()) { // no match - something went wrong
        *errMsg = UnsuppHashType;
        return NULL;
    }
    // check if the peer's commited hash is the same that we used when
    // preparing our commit packet. If not do the necessary resets and
    // recompute some data.
    if (*(int32_t*)(hash->getName()) != *(int32_t*)(cp->getName())) {
        hash = cp;
        setNegotiatedHash(hash);
    }
    myRole = Responder;

    // We are responder. Release a possibly pre-computed SHA256 context
    // because this was prepared for Initiator. Then create a new one.
    if (msgShaContext != NULL) {
        closeHashCtx(msgShaContext, NULL);
    }
    msgShaContext = createHashCtx();

    // Hash messages to produce overall message hash:
    // First the Responder's (my) Hello message, second the Commit 
    // (always Initator's)
    // use negotiated hash
    hashCtxFunction(msgShaContext, (unsigned char*)zrtpHello.getHeaderBase(),
              zrtpHello.getLength() * ZRTP_WORD_SIZE);
    hashCtxFunction(msgShaContext, (unsigned char*)commit->getHeaderBase(),
              commit->getLength() * ZRTP_WORD_SIZE);

    closeHashCtx(msgShaContext, messageHash);
    msgShaContext = NULL;

    generateKeysMultiStream();

    // Fill in Confirm1 packet.
    zrtpConfirm1.setMessageType((uint8_t*)Confirm1Msg);
    // TODO: at this point sas hash is ready, perform signSAS callback and fill in signature if available
    zrtpConfirm1.setSignatureLength(0);
    zrtpConfirm1.setExpTime(0xFFFFFFFF);
    zrtpConfirm1.setIv(randomIV);
    zrtpConfirm1.setHashH0(H0);

    uint8_t confMac[MAX_DIGEST_LENGTH];
    uint32_t macLen;

    // Encrypt and HMAC with Responder's key - we are Respondere here
    int32_t hmlen = (zrtpConfirm1.getLength() - 9) * ZRTP_WORD_SIZE;
	cipher->getEncrypt()(zrtpKeyR, cipher->getKeylen(), randomIV,
                         zrtpConfirm1.getHashH0(), hmlen);
    // Use negotiated HMAC (hash)
    hmacFunction(hmacKeyR, hashLength,
                (unsigned char*)zrtpConfirm1.getHashH0(),
                hmlen, confMac, &macLen);

    zrtpConfirm1.setHmac(confMac);

    // Store Commit data temporarily until we can check HMAC after receiving Confirm2
    storeMsgTemp(commit);
    return &zrtpConfirm1;
}


/*
 * At this point we are Initiator.
 */
ZrtpPacketConfirm* ZRtp::prepareConfirm2(ZrtpPacketConfirm* confirm1, uint32_t* errMsg) {

    sendInfo(Info, InfoInitConf1Received);

    uint8_t confMac[MAX_DIGEST_LENGTH];
    uint32_t macLen;

    // Use the Responder's keys here because we are Initiator here and
    // receive packets from Responder
    int16_t hmlen = (confirm1->getLength() - 9) * ZRTP_WORD_SIZE;

    // Use negotiated HMAC (hash)
    hmacFunction(hmacKeyR, hashLength,
                (unsigned char*)confirm1->getHashH0(),
                hmlen, confMac, &macLen);

    if (memcmp(confMac, confirm1->getHmac(), HMAC_SIZE) != 0) {
        *errMsg = ConfirmHMACWrong;
        return NULL;
    }
	cipher->getDecrypt()(zrtpKeyR, cipher->getKeylen(), 
                         confirm1->getIv(),
                         confirm1->getHashH0(), hmlen);

    std::string cs(cipher->getReadable());
    cs.append("/").append(pubKey->getName());

    // Check HMAC of DHPart1 packet stored in temporary buffer. The
    // HMAC key of the DHPart1 packet is peer's H0 that is contained in
    // Confirm1. Refer to chapter 9.1 and chapter 10.
    if (!checkMsgHmac(confirm1->getHashH0())) {
        sendInfo(Severe, SevereDH1HMACFailed);
        *errMsg = CriticalSWError;
        return NULL;
    }

    // TODO: here we have a SAS signature from reponder, call checkSASsignature (save / compare in case of resend)
    /*
     * The Confirm1 is ok, handle the Retained secret stuff and inform
     * GUI about state.
     */
    bool sasFlag = confirm1->isSASFlag();

    // Initialize a ZID record to get peer's retained secrets
    ZIDRecord zidRec(peerZid);

    ZIDFile *zid = ZIDFile::getInstance();
    zid->getRecord(&zidRec);

    // Our peer did not confirm the SAS in last session, thus reset
    // our SAS flag too.
    if (!sasFlag) {
        zidRec.resetSasVerified();
    }
    // get verified flag from current RS1 before set a new RS1. This
    // may not be set even if peer's flag is set in confirm1 message.
    sasFlag = zidRec.isSasVerified() ? true : false;

    // Inform GUI about security state and SAS state
    bool sasVerified = zidRec.isSasVerified();
    callback->srtpSecretsOn(cs, SAS, sasVerified);

    // now we are ready to save the new RS1 which inherits the verified
    // flag from old RS1
    zidRec.setNewRs1((const uint8_t*)newRs1);
    zid->saveRecord(&zidRec);

    // now generate my Confirm2 message
    zrtpConfirm2.setMessageType((uint8_t*)Confirm2Msg);
    // TODO: set signature data if available
    zrtpConfirm2.setSignatureLength(0);
    zrtpConfirm2.setHashH0(H0); 

    if (sasFlag) {
        zrtpConfirm2.setSASFlag();
    }
    zrtpConfirm2.setExpTime(0xFFFFFFFF);
    zrtpConfirm2.setIv(randomIV);

    // Compute PBX secret if we are in enrollemnt mode (PBX user agent)
    // or enrollment was enabled at normal user agent and flag in confirm packet
    if (enrollmentMode || (enableMitmEnrollment && confirm1->isPBXEnrollment())) {
        computePBXSecret();
         
        // if this runs at PBX user agent enrollment service then set flag in confirm
        // packet and store the MitM key. The PBX user agent service always stores
        // its MitM key.
        if (enrollmentMode) {
            zrtpConfirm2.setPBXEnrollment();
            writeEnrollmentPBX();
        }
    }
    // Encrypt and HMAC with Initiator's key - we are Initiator here
    hmlen = (zrtpConfirm2.getLength() - 9) * ZRTP_WORD_SIZE;
	cipher->getEncrypt()(zrtpKeyI, cipher->getKeylen(), randomIV,
                         zrtpConfirm2.getHashH0(), hmlen); 
    // Use negotiated HMAC (hash)
    hmacFunction(hmacKeyI, hashLength,
                (unsigned char*)zrtpConfirm2.getHashH0(),
                hmlen, confMac, &macLen);

    zrtpConfirm2.setHmac(confMac);

    // Ask for enrollment only if enabled via configuration and the
    // confirm1 packet contains the enrollment flag. The enrolling user
    // agent stores the MitM key only if the user accepts the enrollment
    // request.
    if (enableMitmEnrollment && confirm1->isPBXEnrollment()) {
        callback->zrtpAskEnrollment(EnrollmentRequest);
    }
    return &zrtpConfirm2;
}

/**
 * Save the computed MitM secret to the ZID record of the peer
 */
void ZRtp::writeEnrollmentPBX() {
    // Initialize a ZID record to get peer's retained secrets
    ZIDRecord zidRec(peerZid);

    ZIDFile *zid = ZIDFile::getInstance();
    zid->getRecord(&zidRec);

    if (pbxSecretTmp != NULL) {
        zidRec.setMiTMData(pbxSecretTmp);
    }
    zid->saveRecord(&zidRec);
}

/*
 * At this point we are Initiator.
 */
ZrtpPacketConfirm* ZRtp::prepareConfirm2MultiStream(ZrtpPacketConfirm* confirm1, uint32_t* errMsg) {

    // check Confirm1 packet using the keys
    // prepare Confirm2 packet
    // don't update SAS, RS
    sendInfo(Info, InfoInitConf1Received);

    uint8_t confMac[MAX_DIGEST_LENGTH];
    uint32_t macLen;

    closeHashCtx(msgShaContext, messageHash);
    msgShaContext = NULL;
    myRole = Initiator;

    generateKeysMultiStream();

    // Use the Responder's keys here because we are Initiator here and
    // receive packets from Responder
    int32_t hmlen = (confirm1->getLength() - 9) * ZRTP_WORD_SIZE;

    // Use negotiated HMAC (hash)
    hmacFunction(hmacKeyR, hashLength,
                 (unsigned char*)confirm1->getHashH0(),
                 hmlen, confMac, &macLen);

    if (memcmp(confMac, confirm1->getHmac(), HMAC_SIZE) != 0) {
        *errMsg = ConfirmHMACWrong;
        return NULL;
    }
	cipher->getDecrypt()(zrtpKeyR, cipher->getKeylen(), 
                         confirm1->getIv(),
                         confirm1->getHashH0(), hmlen);
    std::string cs(cipher->getReadable());

    // Because we are initiator the protocol engine didn't receive Commit and
    // because we are using multi-stream mode here we also did not receive a DHPart1 and
    // thus could not store a responder's H2 or H1. A two step hash is required to
    // re-compute H1, H2.
    // USe implicit hash function.
    uint8_t tmpHash[IMPL_MAX_DIGEST_LENGTH];
    hashFunctionImpl(confirm1->getHashH0(), HASH_IMAGE_SIZE, tmpHash); // Compute peer's H1 in tmpHash
    hashFunctionImpl(tmpHash, HASH_IMAGE_SIZE, tmpHash);               // Compute peer's H2 in tmpHash
    memcpy(peerH2, tmpHash, HASH_IMAGE_SIZE);                          // copy and truncate to peerH2

    // Check HMAC of previous Hello packet stored in temporary buffer. The
    // HMAC key of the Hello packet is peer's H2 that was computed above.
    // Refer to chapter 9.1 and chapter 10.
    if (!checkMsgHmac(peerH2)) {
        sendInfo(Severe, SevereHelloHMACFailed);
        *errMsg = CriticalSWError;
        return NULL;
    }
    // TODO: here we have a SAS signature from reponder, call checkSASsignature (save / compare in case of resend)

    // Inform GUI about security state, don't show SAS and its state
    std::string cs1("");
    callback->srtpSecretsOn(cs, cs1, true);

    // now generate my Confirm2 message
    zrtpConfirm2.setMessageType((uint8_t*)Confirm2Msg);
    // TODO: set signature data if available
    zrtpConfirm2.setSignatureLength(0);
    zrtpConfirm2.setHashH0(H0); 
    zrtpConfirm2.setExpTime(0xFFFFFFFF);
    zrtpConfirm2.setIv(randomIV);

    // Encrypt and HMAC with Initiator's key - we are Initiator here
    hmlen = (zrtpConfirm2.getLength() - 9) * ZRTP_WORD_SIZE;
 	cipher->getEncrypt()(zrtpKeyI, cipher->getKeylen(), randomIV,
                         zrtpConfirm2.getHashH0(), hmlen);
    // Use negotiated HMAC (hash)
    hmacFunction(hmacKeyI, hashLength,
                (unsigned char*)zrtpConfirm2.getHashH0(),
                hmlen, confMac, &macLen);

    zrtpConfirm2.setHmac(confMac);
    return &zrtpConfirm2;
}

/*
 * At this point we are Responder.
 */
ZrtpPacketConf2Ack* ZRtp::prepareConf2Ack(ZrtpPacketConfirm *confirm2, uint32_t* errMsg) {

    sendInfo(Info, InfoRespConf2Received);

    uint8_t confMac[MAX_DIGEST_LENGTH];
    uint32_t macLen;

    // Use the Initiator's keys here because we are Responder here and
    // reveice packets from Initiator
    int16_t hmlen = (confirm2->getLength() - 9) * ZRTP_WORD_SIZE;

    // Use negotiated HMAC (hash)
    hmacFunction(hmacKeyI, hashLength,
                (unsigned char*)confirm2->getHashH0(),
                hmlen, confMac, &macLen);

    if (memcmp(confMac, confirm2->getHmac(), HMAC_SIZE) != 0) {
        *errMsg = ConfirmHMACWrong;
        return NULL;
    }
    cipher->getDecrypt()(zrtpKeyI, cipher->getKeylen(),
                         confirm2->getIv(),
                         confirm2->getHashH0(), hmlen);
    
    std::string cs(cipher->getReadable());

    if (!multiStream) {
        // Check HMAC of DHPart2 packet stored in temporary buffer. The
        // HMAC key of the DHPart2 packet is peer's H0 that is contained in
        // Confirm2. Refer to chapter 9.1 and chapter 10.
        if (!checkMsgHmac(confirm2->getHashH0())) {
            sendInfo(Severe, SevereDH2HMACFailed);
            *errMsg = CriticalSWError;
            return NULL;
        }
        /*
        * The Confirm2 is ok, handle the Retained secret stuff and inform
        * GUI about state.
        */
        bool sasFlag = confirm2->isSASFlag();

        // Initialize a ZID record to get peer's retained secrets
        ZIDRecord zidRec(peerZid);

        ZIDFile *zid = ZIDFile::getInstance();
        zid->getRecord(&zidRec);

        // Our peer did not confirm the SAS in last session, thus reset
        // our SAS flag too.
        if (!sasFlag) {
            zidRec.resetSasVerified();
        }

        // Inform GUI about security state and SAS state
        bool sasVerified = zidRec.isSasVerified();
        cs.append("/").append(pubKey->getName());
        callback->srtpSecretsOn(cs, SAS, sasVerified);

        // save new RS1, this inherits the verified flag from old RS1
        zidRec.setNewRs1((const uint8_t*)newRs1);
        zid->saveRecord(&zidRec);
            
        // Ask for enrollment only if enabled via configuration and the
        // confirm packet contains the enrollment flag. The enrolling user
        // agent stores the MitM key only if the user accepts the enrollment
        // request.
        if (enableMitmEnrollment && confirm2->isPBXEnrollment()) {
            computePBXSecret();
            callback->zrtpAskEnrollment(EnrollmentRequest);
        }
    }
    else {
        // Check HMAC of Commit packet stored in temporary buffer. The
        // HMAC key of the Commit packet is initiator's H1
        // use implicit hash function.
        uint8_t tmpHash[IMPL_MAX_DIGEST_LENGTH];
        hashFunctionImpl(confirm2->getHashH0(), HASH_IMAGE_SIZE, tmpHash); // Compute initiator's H1 in tmpHash

        if (!checkMsgHmac(tmpHash)) {
            sendInfo(Severe, SevereCommitHMACFailed);
            *errMsg = CriticalSWError;
            return NULL;
        }
        std::string cs1("");
        // Inform GUI about security state, don't show SAS and its state
        callback->srtpSecretsOn(cs, cs1, true);
    }
    // TODO: clear responders signature data in, prepare a call to do same for initiaor (called by state engine)
    return &zrtpConf2Ack;
}

ZrtpPacketErrorAck* ZRtp::prepareErrorAck(ZrtpPacketError* epkt) {
    sendInfo(ZrtpError, epkt->getErrorCode() * -1);
    return &zrtpErrorAck;
}

ZrtpPacketError* ZRtp::prepareError(uint32_t errMsg) {
    zrtpError.setErrorCode(errMsg);
    return &zrtpError;
}

ZrtpPacketPingAck* ZRtp::preparePingAck(ZrtpPacketPing* ppkt) {

    // Because we do not support ZRTP proxy mode use the truncated ZID.
    // If this code shall be used in ZRTP proxy implementation the computation
    // of the endpoint hash must be enhanced (see chaps 5.15ff and 5.16)
    zrtpPingAck.setLocalEpHash(zid);
    zrtpPingAck.setRemoteEpHash(ppkt->getEpHash());
    zrtpPingAck.setSSRC(peerSSRC);
    return &zrtpPingAck;
}

ZrtpPacketRelayAck* ZRtp::prepareRelayAck(ZrtpPacketSASrelay* srly, uint32_t* errMsg) {
    if (!mitmSeen)
        return &zrtpRelayAck;

    uint8_t* hkey, *ekey;
    // If we are responder then the PBX used it's Initiator keys
    if (myRole == Responder) {
        hkey = hmacKeyI;
        ekey = zrtpKeyI;
    }
    else {
        hkey = hmacKeyR;
        ekey = zrtpKeyR;
    }

    uint8_t confMac[MAX_DIGEST_LENGTH];
    uint32_t macLen;

    // Use the Initiator's keys here because we are Responder here and
    // reveice packets from Initiator
    int16_t hmlen = (srly->getLength() - 9) * ZRTP_WORD_SIZE;

    // Use negotiated HMAC (hash)
    hmacFunction(hkey, hashLength,
                 (unsigned char*)srly->getFiller(),
                 hmlen, confMac, &macLen);

    if (memcmp(confMac, srly->getHmac(), HMAC_SIZE) != 0) {
        *errMsg = ConfirmHMACWrong;
        return NULL;                // TODO - check error handling
    }
    cipher->getDecrypt()(ekey, cipher->getKeylen(),
                         srly->getIv(),
                         (uint8_t*)srly->getFiller(), hmlen);

    const uint8_t* render = srly->getSas();
    const uint8_t* newSasHash = srly->getTrustedSas();

    bool sasHashNull = true;
    for (int i = 0; i < HASH_IMAGE_SIZE; i++) {
        if (newSasHash[i] != 0) {
            sasHashNull = false;
            break;
        }
    }
    // if the new SAS is not null then we need a trusted MitM. If this is not
    // the case then don't render the new SAS - return.
    if (!sasHashNull && !trustedMitM) {
        return &zrtpRelayAck;
    }
    // If other SAS schemes required - check here and use others
    AlgorithmEnum* renderAlgo = &zrtpSasTypes.getByName((const char*)render);
    if (renderAlgo->isValid()) {
        uint8_t sasBytes[4];;
        if (!sasHashNull) {
            sasBytes[0] = newSasHash[0];
            sasBytes[1] = newSasHash[1];
            sasBytes[2] = newSasHash[2] & 0xf0;
            sasBytes[3] = 0;
        }
        else {
            sasBytes[0] = sasHash[0];
            sasBytes[1] = sasHash[1];
            sasBytes[2] = sasHash[2] & 0xf0;
            sasBytes[3] = 0;
        }
        SAS = Base32(sasBytes, 20).getEncoded();
    }
    std::string cs(cipher->getReadable());
    cs.append("/").append(pubKey->getName()).append("/MitM");

    callback->srtpSecretsOn(cs, SAS, false);
    return &zrtpRelayAck;
}

// TODO Implement GoClear handling
ZrtpPacketClearAck* ZRtp::prepareClearAck(ZrtpPacketGoClear* gpkt) {
    sendInfo(Warning, WarningGoClearReceived);
    return &zrtpClearAck;
}

ZrtpPacketGoClear* ZRtp::prepareGoClear(uint32_t errMsg) {
    ZrtpPacketGoClear* gclr = &zrtpGoClear;
    gclr->clrClearHmac();
    return gclr;
}

/*
 * The next functions look up and return a prefered algorithm. These
 * functions work as follows:
 * - If the Hello packet does not contain an algorithm (number of algorithms 
*    is zero) then return the mandatory algorithm.
 * - Build a list of algorithm names and ids from configuration data. If
 *   the configuration data does not contain a mandatory algorithm append
 *   the mandatory algorithm to the list and ids.
 * - Build a list of algorithm names from the Hello message. If
 *   the Hello message does not contain a mandatory algorithm append
 *   the mandatory algorithm to the list.
 * - Lookup a matching algorithm. The list built from Hello takes
 *   precedence in the lookup (indexed by the outermost loop).
 *
 * This guarantees that we always return a supported alogrithm respecting
 * the order of algorithms in the Hello message
 *
 * The mandatory algorithms are: (internal enums are our prefered algoritms)
 * Hash:                S256 (SHA 256)             (internal enum Sha256)
 * Symmetric Cipher:    AES1 (AES 128)             (internal enum Aes128)
 * SRTP Authentication: HS32 and HS80 (32/80 bits) (internal enum AuthLen32)
 * Key Agreement:       DH3k (3072 Diffie-Helman)  (internal enum Dh3072)
 *
 */
AlgorithmEnum* ZRtp::findBestHash(ZrtpPacketHello *hello) {

    int i;
    int ii;
    int numAlgosOffered;
    AlgorithmEnum* algosOffered[ZrtpConfigure::maxNoOfAlgos+1];

    int numAlgosConf;
    AlgorithmEnum* algosConf[ZrtpConfigure::maxNoOfAlgos+1];

    bool mandatoryFound = false;

    // If Hello does not contain any hash names return Sha256, its mandatory
    int num = hello->getNumHashes();
    if (num == 0) {
        return &zrtpHashes.getByName(mandatoryHash);
    }
    // Build list of configured hash algorithm names, append mandatory algos
    // if necessary.
    numAlgosConf = configureAlgos.getNumConfiguredAlgos(HashAlgorithm);
    for (i = 0; i < numAlgosConf; i++) {
        algosConf[i] = &configureAlgos.getAlgoAt(HashAlgorithm, i);
        if (*(int32_t*)(algosConf[i]->getName()) == *(int32_t*)mandatoryHash) {
            mandatoryFound = true;
        }
    }
    if (!mandatoryFound) {
        algosConf[numAlgosConf++] = &zrtpHashes.getByName(mandatoryHash);
    }

    // Build list of offered known algos in Hello, append mandatory algos if necessary
    mandatoryFound = false;
    for (numAlgosOffered = 0, i = 0; i < num; i++) {
        algosOffered[numAlgosOffered] = &zrtpHashes.getByName((const char*)hello->getHashType(i));
        if(!algosOffered[numAlgosOffered]->isValid())
            continue;
        if (*(int32_t*)(algosOffered[numAlgosOffered++]->getName()) == *(int32_t*)mandatoryHash) {
            mandatoryFound = true;
        }
    }
    if (!mandatoryFound) {
        algosOffered[numAlgosOffered++] = &zrtpHashes.getByName(mandatoryHash);
    }

    // Lookup offered algos in configured algos. Because of appended
    // mandatory algorithms at least one match will happen
    for (i = 0; i < numAlgosOffered; i++) {
        for (ii = 0; ii < numAlgosConf; ii++) {
            if (*(int32_t*)(algosOffered[i]->getName()) == *(int32_t*)(algosConf[ii]->getName())) {
                return algosConf[ii];
            }
        }
    }
    return &zrtpHashes.getByName(mandatoryHash);
}

AlgorithmEnum* ZRtp::findBestCipher(ZrtpPacketHello *hello, AlgorithmEnum* pk) {

    int i;
    int ii;
    int numAlgosOffered;
    AlgorithmEnum* algosOffered[ZrtpConfigure::maxNoOfAlgos+1];

    int numAlgosConf;
    AlgorithmEnum* algosConf[ZrtpConfigure::maxNoOfAlgos+1];

    bool mandatoryFound = false;

    int num = hello->getNumCiphers();
    if (num == 0 || (*(int32_t*)(pk->getName()) == *(int32_t*)dh2k)) {
        return &zrtpSymCiphers.getByName(aes1);
    }

    // Build list of configured cipher algorithm names, append mandatory algos
    // if necessary.
    numAlgosConf = configureAlgos.getNumConfiguredAlgos(CipherAlgorithm);
    for (i = 0; i < numAlgosConf; i++) {
        algosConf[i] = &configureAlgos.getAlgoAt(CipherAlgorithm, i);
        if (*(int32_t*)(algosConf[i]->getName()) == *(int32_t*)mandatoryCipher) {
            mandatoryFound = true;
        }
    }
    if (!mandatoryFound) {
        algosConf[numAlgosConf++] = &zrtpSymCiphers.getByName(mandatoryCipher);
    }

    // Build list of offered known algos names in Hello, append mandatory algos if
    // necessary
    mandatoryFound = false;
    for (numAlgosOffered = 0, i = 0; i < num; i++) {
        algosOffered[numAlgosOffered] = &zrtpSymCiphers.getByName((const char*)hello->getCipherType(i));
        if(!algosOffered[numAlgosOffered]->isValid())
            continue;
        if (*(int32_t*)(algosOffered[numAlgosOffered++]->getName()) == *(int32_t*)mandatoryCipher) {
            mandatoryFound = true;
        }
    }

    if (!mandatoryFound) {
        algosOffered[numAlgosOffered++] = &zrtpSymCiphers.getByName(mandatoryCipher);
    }

    // Lookup offered algos in configured algos. Because of appended
    // mandatory algorithms at least one match will happen
    for (i = 0; i < numAlgosOffered; i++) {
        for (ii = 0; ii < numAlgosConf; ii++) {
            if (*(int32_t*)(algosOffered[i]->getName()) == *(int32_t*)(algosConf[ii]->getName())) {
                return algosConf[ii];
            }
        }
    }
    return &zrtpSymCiphers.getByName(mandatoryCipher);
}

AlgorithmEnum* ZRtp::findBestPubkey(ZrtpPacketHello *hello) {

    int i;
    int ii;
    int numAlgosOffered;
    AlgorithmEnum* algosOffered[ZrtpConfigure::maxNoOfAlgos+1];

    int numAlgosConf;
    AlgorithmEnum* algosConf[ZrtpConfigure::maxNoOfAlgos+1];

    bool mandatoryFound = false;

    int num = hello->getNumPubKeys();
    if (num == 0) {
        return &zrtpPubKeys.getByName(mandatoryPubKey);
    }
    // Build list of configured pubkey algorithm names, append mandatory algos
    // if necessary.
    // The list must include real public key algorithms only, so skip
    // mult-stream mode, preshared and alike.
    numAlgosConf = configureAlgos.getNumConfiguredAlgos(PubKeyAlgorithm);
    for (i = 0, ii = 0; i < numAlgosConf; i++) {
        algosConf[ii] = &configureAlgos.getAlgoAt(PubKeyAlgorithm, ii);
        if (*(int32_t*)(algosConf[ii]->getName()) == *(int32_t*)mult) {
            continue;                               // skip multi-stream mode
        }
        if (*(int32_t*)(algosConf[ii++]->getName()) == *(int32_t*)mandatoryPubKey) {
            mandatoryFound = true;
        }
    }

    numAlgosConf = ii;
    if (!mandatoryFound) {
        algosConf[numAlgosConf++] = &zrtpPubKeys.getByName(mandatoryPubKey);
    }

    // Build list of offered known algos in Hello, append mandatory algos if necessary
    mandatoryFound = false;
    for (numAlgosOffered = 0, i = 0; i < num; i++) {
        algosOffered[numAlgosOffered] = &zrtpPubKeys.getByName((const char*)hello->getPubKeyType(i));
        if(!algosOffered[numAlgosOffered]->isValid())
            continue;
        if (*(int32_t*)(algosOffered[numAlgosOffered++]->getName()) == *(int32_t*)mandatoryPubKey) {
            mandatoryFound = true;
        }
    }

    if (!mandatoryFound) {
        algosOffered[numAlgosOffered++] = &zrtpPubKeys.getByName(mandatoryPubKey);
    }

    // Lookup offered algos in configured algos. Because of appended
    // mandatory algorithms at least one match will happen
    for (i = 0; i < numAlgosOffered; i++) {
        for (ii = 0; ii < numAlgosConf; ii++) {
            if (*(int32_t*)(algosOffered[i]->getName()) == *(int32_t*)(algosConf[ii]->getName())) {
                return algosConf[ii];
            }
        }
    }
    return &zrtpPubKeys.getByName(mandatoryPubKey);
}

AlgorithmEnum* ZRtp::findBestSASType(ZrtpPacketHello *hello) {

    int  i;
    int ii;
    int numAlgosOffered;
    AlgorithmEnum* algosOffered[ZrtpConfigure::maxNoOfAlgos+1];

    int numAlgosConf;
    AlgorithmEnum* algosConf[ZrtpConfigure::maxNoOfAlgos+1];

    bool mandatoryFound = false;

    int num = hello->getNumSas();
    if (num == 0) {
        return &zrtpSasTypes.getByName(mandatorySasType);
    }
    // Buildlist of configured SAS algorithm names, append mandatory algos
    // if necessary.
    numAlgosConf = configureAlgos.getNumConfiguredAlgos(SasType);
    for (i = 0; i < numAlgosConf; i++) {
        algosConf[i] = &configureAlgos.getAlgoAt(SasType, i);
        if (*(int32_t*)(algosConf[i]->getName()) == *(int32_t*)mandatorySasType) {
            mandatoryFound = true;
        }
    }

    if (!mandatoryFound) {
        algosConf[numAlgosConf++] = &zrtpSasTypes.getByName(mandatorySasType);
    }

    // Build list of offered known algos in Hello, append mandatory algos if necessary
    for (numAlgosOffered = 0, i = 0; i < num; i++) {
        algosOffered[numAlgosOffered] = &zrtpSasTypes.getByName((const char*)hello->getSasType(i++));
        if(!algosOffered[numAlgosOffered]->isValid())
            continue;
        if (*(int32_t*)(algosOffered[numAlgosOffered++]->getName()) == *(int32_t*)mandatorySasType) {
            mandatoryFound = true;
        }
    }

    if (!mandatoryFound) {
        algosOffered[numAlgosOffered++] = &zrtpSasTypes.getByName(mandatorySasType);
    }

    // Lookup offered algos in configured algos. Because of appended
    // mandatory algorithms at least one match will happen
    for (i = 0; i < numAlgosOffered; i++) {
        for (ii = 0; ii < numAlgosConf; ii++) {
            if (*(int32_t*)(algosOffered[i]->getName()) == *(int32_t*)(algosConf[ii]->getName())) {
                return algosConf[ii];
            }
        }
    }
    return &zrtpSasTypes.getByName(mandatorySasType);
}

AlgorithmEnum* ZRtp::findBestAuthLen(ZrtpPacketHello *hello) {

    int  i;
    int ii;
    int numAlgosOffered;
    AlgorithmEnum* algosOffered[ZrtpConfigure::maxNoOfAlgos+2];

    int numAlgosConf;
    AlgorithmEnum* algosConf[ZrtpConfigure::maxNoOfAlgos+2];

    bool mandatoryFound_1 = false;
    bool mandatoryFound_2 = false;

    int num = hello->getNumAuth();
    if (num == 0) {
        return &zrtpAuthLengths.getByName(mandatoryAuthLen_1);
    }

    // Build list of configured SAS algorithm names, append mandatory algos
    // if necessary.
    numAlgosConf = configureAlgos.getNumConfiguredAlgos(AuthLength);
    for (i = 0; i < numAlgosConf; i++) {
        algosConf[i] = &configureAlgos.getAlgoAt(AuthLength, i);
        if (*(int32_t*)(algosConf[i]->getName()) == *(int32_t*)mandatoryAuthLen_1) {
            mandatoryFound_1 = true;
        }
        if (*(int32_t*)(algosConf[i]->getName()) == *(int32_t*)mandatoryAuthLen_2) {
            mandatoryFound_2 = true;
        }
    }

    if (!mandatoryFound_1) {
        algosConf[numAlgosConf++] = &zrtpAuthLengths.getByName(mandatoryAuthLen_1);
    }

    if (!mandatoryFound_2) {
        algosConf[numAlgosConf++] = &zrtpAuthLengths.getByName(mandatoryAuthLen_2);
    }

    // Build list of offered known algos in Hello, append mandatory algos if necessary
    for (numAlgosOffered = 0, i = 0; i < num; i++) {
        algosOffered[numAlgosOffered] = &zrtpAuthLengths.getByName((const char*)hello->getAuthLen(i));
        if(!algosOffered[numAlgosOffered]->isValid())
            continue;
        if (*(int32_t*)(algosOffered[numAlgosOffered]->getName()) == *(int32_t*)mandatoryAuthLen_1) {
            mandatoryFound_1 = true;
        }
        if (*(int32_t*)(algosOffered[numAlgosOffered++]->getName()) == *(int32_t*)mandatoryAuthLen_2) {
            mandatoryFound_2 = true;
        }
    }
    if (!mandatoryFound_1) {
        algosOffered[numAlgosOffered++] = &zrtpAuthLengths.getByName(mandatoryAuthLen_1);
    }
    if (!mandatoryFound_2) {
        algosOffered[numAlgosOffered++] = &zrtpAuthLengths.getByName(mandatoryAuthLen_2);
    }
    // Lookup offered algos in configured algos. Because of appended
    // mandatory algorithms at least one match will happen
    for (i = 0; i < numAlgosOffered; i++) {
        for (ii = 0; ii < numAlgosConf; ii++) {
            if (*(int32_t*)(algosOffered[i]->getName()) == *(int32_t*)(algosConf[ii]->getName())) {
                return algosConf[ii];
            }
        }
    }
    return &zrtpAuthLengths.getByName(mandatoryAuthLen_1);
}

bool ZRtp::checkMultiStream(ZrtpPacketHello *hello) {

    int  i;
    int num = hello->getNumPubKeys();

    // Multi Stream mode is mandatory, thus if nothing is offered then it is supported :-)
    if (num == 0) {
        return true;
    }
    for (i = 0; i < num; i++) {
        if (*(int32_t*)(hello->getPubKeyType(i)) == *(int32_t*)mult) {
            return true;
        }
    }
    return false;
}

bool ZRtp::verifyH2(ZrtpPacketCommit *commit) {
    uint8_t tmpH3[IMPL_MAX_DIGEST_LENGTH];

    sha256(commit->getH2(), HASH_IMAGE_SIZE, tmpH3);
    if (memcmp(tmpH3, peerH3, HASH_IMAGE_SIZE) != 0) {
        return false;
    }
    return true;
}

void ZRtp::computeHvi(ZrtpPacketDHPart* dh, ZrtpPacketHello *hello) {

    unsigned char* data[3];
    unsigned int length[3];
    /*
     * populate the vector to compute the HVI hash according to the
     * ZRTP specification.
     */
    data[0] = (uint8_t*)dh->getHeaderBase();
    length[0] = dh->getLength() * ZRTP_WORD_SIZE;

    data[1] = (uint8_t*)hello->getHeaderBase();
    length[1] = hello->getLength() * ZRTP_WORD_SIZE;

    data[2] = NULL;            // terminate data chunks
    hashListFunction(data, length, hvi);
    return;
}

void ZRtp:: computeSharedSecretSet(ZIDRecord &zidRec) {

   /*
    * Compute the Initiator's and Reponder's retained shared secret Ids.
    * Use negotiated HMAC.
    */
    uint8_t randBuf[RS_LENGTH];
    uint32_t macLen;

    if (!zidRec.isRs1Valid()) {
	randomZRTP(randBuf, RS_LENGTH);
	hmacFunction(randBuf, RS_LENGTH, (unsigned char*)initiator,
		    strlen(initiator), rs1IDi, &macLen);
	hmacFunction(randBuf, RS_LENGTH, (unsigned char*)responder,
		    strlen(responder), rs1IDr, &macLen);
    }
    else {
        rs1Valid = true;
	hmacFunction((unsigned char*)zidRec.getRs1(), RS_LENGTH,
		     (unsigned char*)initiator, strlen(initiator),
		     rs1IDi, &macLen);

	hmacFunction((unsigned char*)zidRec.getRs1(), RS_LENGTH,
		     (unsigned char*)responder, strlen(responder),
		     rs1IDr, &macLen);
    }

    if (!zidRec.isRs2Valid()) {
	randomZRTP(randBuf, RS_LENGTH);
	hmacFunction(randBuf, RS_LENGTH, (unsigned char*)initiator,
		    strlen(initiator), rs2IDi, &macLen);
	hmacFunction(randBuf, RS_LENGTH, (unsigned char*)responder,
		    strlen(responder), rs2IDr, &macLen);
    }
    else {
        rs2Valid = true;
	hmacFunction((unsigned char*)zidRec.getRs2(), RS_LENGTH,
		     (unsigned char*)initiator, strlen(initiator),
		     rs2IDi, &macLen);
	hmacFunction((unsigned char*)zidRec.getRs2(), RS_LENGTH,
		     (unsigned char*)responder, strlen(responder),
		     rs2IDr, &macLen);
    }

    /*
    * For the time being we don't support this types of shared secrect. Could be
    * easily done: somebody sets some data into our ZRtp object, check it here
    * and use it. Otherwise use the random data.
    */
    randomZRTP(randBuf, RS_LENGTH);
    hmacFunction(randBuf, RS_LENGTH, (unsigned char*)initiator,
		strlen(initiator), auxSecretIDi, &macLen);
    hmacFunction(randBuf, RS_LENGTH, (unsigned char*)responder,
		strlen(responder), auxSecretIDr, &macLen);

    
    if (!zidRec.isMITMKeyAvailable()) {
        randomZRTP(randBuf, RS_LENGTH);
        hmacFunction(randBuf, RS_LENGTH, (unsigned char*)initiator,
                     strlen(initiator), pbxSecretIDi, &macLen);
        hmacFunction(randBuf, RS_LENGTH, (unsigned char*)responder,
                     strlen(responder), pbxSecretIDr, &macLen);
    }
    else {
        hmacFunction((unsigned char*)zidRec.getMiTMData(), RS_LENGTH, (unsigned char*)initiator,
                     strlen(initiator), pbxSecretIDi, &macLen);
        hmacFunction((unsigned char*)zidRec.getMiTMData(), RS_LENGTH, (unsigned char*)responder,
                     strlen(responder), pbxSecretIDr, &macLen);
    }
}

/*
 * The DH packet for this function is DHPart1 and contains the Responder's
 * retained secret ids. Compare them with the expected secret ids (refer
 * to chapter 5.3 in the specification).
 * When using this method then we are in Initiator role.
 */
void ZRtp::generateKeysInitiator(ZrtpPacketDHPart *dhPart, ZIDRecord& zidRec) {
    const uint8_t* setD[3];
    int32_t rsFound = 0;

    setD[0] = setD[1] = setD[2] = NULL;

    /*
     * Select the real secrets into setD. The dhPart is DHpart1 message 
     * received from responder. rs1IDr and rs2IDr are the expected ids using
     * the initator's cached retained secrets.
     */
    int matchingSecrets = 0;
    if (memcmp(rs1IDr, dhPart->getRs1Id(), HMAC_SIZE) == 0) {
        setD[matchingSecrets++] = zidRec.getRs1();
        rsFound = 0x1;
    } 
    else if (memcmp(rs1IDr, dhPart->getRs2Id(), HMAC_SIZE) == 0) {
        setD[matchingSecrets++] = zidRec.getRs1();
        rsFound = 0x2;
    }
    else if (memcmp(rs2IDr, dhPart->getRs1Id(), HMAC_SIZE) == 0) {
        setD[matchingSecrets++] = zidRec.getRs2();
        rsFound = 0x4;
    }
    else if (memcmp(rs2IDr, dhPart->getRs2Id(), HMAC_SIZE) == 0) {
        setD[matchingSecrets++] = zidRec.getRs2();
        rsFound = 0x8;
    }
    /* *** Not yet supported 
    if (memcmp(auxSecretIDr, dhPart->getAuxSecretId(), 8) == 0) {
	DEBUGOUT((fprintf(stdout, "%c: Match for aux secret found\n", zid[0])));
        setD[matchingSecrets++] = auxSecret;
    }
    */
    if (memcmp(pbxSecretIDr, dhPart->getPbxSecretId(), 8) == 0) {
        DEBUGOUT((fprintf(stdout, "%c: Match for Other_secret found\n", zid[0])));
        setD[matchingSecrets++] = zidRec.getMiTMData();
    }
    // Check if some retained secrets found
    if (rsFound == 0) {                        // no RS matches found
        if (rs1Valid || rs2Valid) {            // but valid RS records in cache
            sendInfo(Warning, WarningNoExpectedRSMatch);
            zidRec.resetSasVerified();
        }
        else {                                 // No valid RS record in cache
            sendInfo(Warning, WarningNoRSMatch);
        }
    }
    else {                                     // at least one RS matches
        sendInfo(Info, InfoRSMatchFound);
    }
    /*
     * Ready to generate s0 here.
     * The formular to compute S0 (Refer to ZRTP specification 5.4.4):
     *
      s0 = hash( counter | DHResult | "ZRTP-HMAC-KDF" | ZIDi | ZIDr | \
      total_hash | len(s1) | s1 | len(s2) | s2 | len(s3) | s3)
     *
     * Note: in this function we are Initiator, thus ZIDi is our zid 
     * (zid), ZIDr is the peer's zid (peerZid).
     */

    /*
     * These arrays hold the pointers and lengths of the data that must be
     * hashed to create S0.  According to the formula the max number of 
     * elements to hash is 12, add one for the terminating "NULL"
     */
    unsigned char* data[13];
    unsigned int   length[13];
    uint32_t pos = 0;                  // index into the array

    // we need a number of length data items, so define them here 
    uint32_t counter, sLen[3];

    //Very first element is a fixed counter, big endian 
    counter = 1;
    counter = htonl(counter);
    data[pos] = (unsigned char*)&counter;
    length[pos++] = sizeof(uint32_t);

    // Next is the DH result itself
    data[pos] = DHss;
    length[pos++] = dhContext->getDhSize();

    // Next the fixed string "ZRTP-HMAC-KDF"
    data[pos] = (unsigned char*)KDFString;
    length[pos++] = strlen(KDFString);

    // Next is Initiator's id (ZIDi), in this case as Initiator 
    // it is zid 
    data[pos] = zid;
    length[pos++] = ZID_SIZE;

    // Next is Responder's id (ZIDr), in this case our peer's id 
    data[pos] = peerZid;
    length[pos++] = ZID_SIZE;

    // Next ist total hash (messageHash) itself 
    data[pos] = messageHash;
    length[pos++] = hashLength;

    /*
     * For each matching shared secret hash the length of 
     * the shared secret as 32 bit big-endian number followd by the 
     * shared secret itself. The length of a shared seceret is 
     * currently fixed to RS_LENGTH. If a shared 
     * secret is not used _only_ its length is hased as zero 
     * length. NOTE: if implementing auxSecret and/or pbxSecret -> check
     * this length stuff again.
     */
    int secretHashLen = RS_LENGTH;
    secretHashLen = htonl(secretHashLen);        // prepare 32 bit big-endian number 

    for (int32_t i = 0; i < 3; i++) {
        if (setD[i] != NULL) {           // a matching secret, set length, then secret
            sLen[i] = secretHashLen;
            data[pos] = (unsigned char*)&sLen[i];
            length[pos++] = sizeof(uint32_t);
            data[pos] = (unsigned char*)setD[i];
            length[pos++] = RS_LENGTH;
        }
        else {                           // no machting secret, set length 0, skip secret
            sLen[i] = 0;
            data[pos] = (unsigned char*)&sLen[i];
            length[pos++] = sizeof(uint32_t);
        }
    }

    data[pos] = NULL;
    hashListFunction(data, length, s0);
//  hexdump("S0 I", s0, hashLength);

    memset(DHss, 0, dhContext->getDhSize());
    delete[] DHss;
    DHss = NULL;

    computeSRTPKeys();
    memset(s0, 0, MAX_DIGEST_LENGTH);
}
/*
 * The DH packet for this function is DHPart2 and contains the Initiator's
 * retained secret ids. Compare them with the expected secret ids (refer
 * to chapter 5.3.1 in the specification).
 */
void ZRtp::generateKeysResponder(ZrtpPacketDHPart *dhPart, ZIDRecord& zidRec) {
    const uint8_t* setD[3];
    int32_t rsFound = 0;

    setD[0] = setD[1] = setD[2] = NULL;

    /*
     * Select the real secrets into setD
     */
    int matchingSecrets = 0;
    if (memcmp(rs1IDi, dhPart->getRs1Id(), HMAC_SIZE) == 0) {
        setD[matchingSecrets++] = zidRec.getRs1();
        rsFound = 0x1;
    }
    else if (memcmp(rs1IDi, dhPart->getRs2Id(), HMAC_SIZE) == 0) {
        setD[matchingSecrets++] = zidRec.getRs1();
        rsFound = 0x2;
    }
    else if (memcmp(rs2IDi, dhPart->getRs2Id(), HMAC_SIZE) == 0) {
        setD[matchingSecrets++] = zidRec.getRs2();
        rsFound |= 0x4;
    }
    else if (memcmp(rs2IDi, dhPart->getRs1Id(), HMAC_SIZE) == 0) {
        setD[matchingSecrets++] = zidRec.getRs2();
        rsFound |= 0x8;
    }
    /* ***** not yet supported
    if (memcmp(auxSecretIDi, dhPart->getauxSecretId(), 8) == 0) {
	DEBUGOUT((fprintf(stdout, "%c: Match for aux secret found\n", zid[0])));
        setD[matchingSecrets++] = ;
    }
    */
    if (memcmp(pbxSecretIDi, dhPart->getPbxSecretId(), 8) == 0) {
        DEBUGOUT((fprintf(stdout, "%c: Match for PBX secret found\n", zid[0])));
        setD[matchingSecrets++] = zidRec.getMiTMData();
    }
    // Check if some retained secrets found
    if (rsFound == 0) {                        // no RS matches found
        if (rs1Valid || rs2Valid) {            // but valid RS records in cache
            sendInfo(Warning, WarningNoExpectedRSMatch);
            zidRec.resetSasVerified();
        }
        else {                                 // No valid RS record in cache
            sendInfo(Warning, WarningNoRSMatch);
        }
    }
    else {                                     // at least one RS matches
        sendInfo(Info, InfoRSMatchFound);
    }

    /*
     * ready to generate s0 here.
     * The formular to compute S0 (Refer to ZRTP specification 5.4.4):
     *
      s0 = hash( counter | DHResult | "ZRTP-HMAC-KDF" | ZIDi | ZIDr | \
      total_hash | len(s1) | s1 | len(s2) | s2 | len(s3) | s3)
     *
     * Note: in this function we are Responder, thus ZIDi is the peer's zid 
     * (peerZid), ZIDr is our zid.
     */

    /*
     * These arrays hold the pointers and lengths of the data that must be
     * hashed to create S0.  According to the formula the max number of 
     * elements to hash is 12, add one for the terminating "NULL"
     */
    unsigned char* data[13];
    unsigned int   length[13];
    uint32_t pos = 0;                  // index into the array


    // we need a number of length data items, so define them here 
    uint32_t counter, sLen[3];

    //Very first element is a fixed counter, big endian 
    counter = 1;
    counter = htonl(counter);
    data[pos] = (unsigned char*)&counter;
    length[pos++] = sizeof(uint32_t);

    // Next is the DH result itself
    data[pos] = DHss;
    length[pos++] = dhContext->getDhSize();

    // Next the fixed string "ZRTP-HMAC-KDF"
    data[pos] = (unsigned char*)KDFString;
    length[pos++] = strlen(KDFString);

    // Next is Initiator's id (ZIDi), in this case as Responder 
    // it is peerZid 
    data[pos] = peerZid;
    length[pos++] = ZID_SIZE;

    // Next is Responder's id (ZIDr), in this case our own zid 
    data[pos] = zid;
    length[pos++] = ZID_SIZE;

    // Next ist total hash (messageHash) itself 
    data[pos] = messageHash;
    length[pos++] = hashLength;

    /*
     * For each matching shared secret hash the length of 
     * the shared secret as 32 bit big-endian number followd by the 
     * shared secret itself. The length of a shared seceret is 
     * currently fixed to SHA256_DIGEST_LENGTH. If a shared 
     * secret is not used _only_ its length is hased as zero 
     * length. NOTE: if implementing auxSecret and/or pbxSecret -> check
     * this length stuff again.
     */
    int secretHashLen = RS_LENGTH;
    secretHashLen = htonl(secretHashLen);        // prepare 32 bit big-endian number 

    for (int32_t i = 0; i < 3; i++) {
        if (setD[i] != NULL) {           // a matching secret, set length, then secret
            sLen[i] = secretHashLen;
            data[pos] = (unsigned char*)&sLen[i];
            length[pos++] = sizeof(uint32_t);
            data[pos] = (unsigned char*)setD[i];
            length[pos++] = RS_LENGTH;
        }
        else {                           // no machting secret, set length 0, skip secret
            sLen[i] = 0;
            data[pos] = (unsigned char*)&sLen[i];
            length[pos++] = sizeof(uint32_t);
        }
    }

    data[pos] = NULL;
    hashListFunction(data, length, s0);
//  hexdump("S0 R", s0, hashLength);

    memset(DHss, 0, dhContext->getDhSize());
    delete[] DHss;
    DHss = NULL;

    computeSRTPKeys();
    memset(s0, 0, MAX_DIGEST_LENGTH);
}


void ZRtp::KDF(uint8_t* key, uint32_t keyLength, uint8_t* label, int32_t labelLength,
               uint8_t* context, int32_t contextLength, int32_t L, uint8_t* output) {

    unsigned char* data[6];
    uint32_t length[6];
    uint32_t pos = 0;                  // index into the array
    uint32_t maclen = 0;

    // Very first element is a fixed counter, big endian 
    uint32_t counter = 1;
    counter = htonl(counter);
    data[pos] = (unsigned char*)&counter;
    length[pos++] = sizeof(uint32_t);

    // Next element is the label, null terminated, labelLength includes null byte.
    data[pos] = label;
    length[pos++] = labelLength;

    // Next is the KDF context
    data[pos] = context;
    length[pos++] = contextLength;

    // last element is HMAC length in bits, big endian
    uint32_t len = htonl(L);
    data[pos] = (unsigned char*)&len;
    length[pos++] = sizeof(uint32_t);

    data[pos] = NULL;

    // Use negotiated hash.
    // TODO: variable digest length
    hmacListFunction(key, keyLength, data, length, output, &maclen);
}

// Compute the Multi Stream mode s0
void ZRtp::generateKeysMultiStream() {

    // allocate the maximum size, compute real size to use
    uint8_t KDFcontext[sizeof(peerZid)+sizeof(zid)+sizeof(messageHash)];
    int32_t kdfSize = sizeof(peerZid)+sizeof(zid)+hashLength;

    if (myRole == Responder) {
        memcpy(KDFcontext, peerZid, sizeof(peerZid));
        memcpy(KDFcontext+sizeof(peerZid), zid, sizeof(zid));
    }
    else {
        memcpy(KDFcontext, zid, sizeof(zid));
        memcpy(KDFcontext+sizeof(zid), peerZid, sizeof(peerZid));
    }
    memcpy(KDFcontext+sizeof(zid)+sizeof(peerZid), messageHash, hashLength);

    KDF(zrtpSession, hashLength, (unsigned char*)zrtpMsk, strlen(zrtpMsk)+1,
        KDFcontext, kdfSize, hashLength*8, s0);

    memset(KDFcontext, 0, sizeof(KDFcontext));

    computeSRTPKeys();
}

void ZRtp::computePBXSecret() {
        // Construct the KDF context as per ZRTP specification chap 7.3.1:
        // ZIDi || ZIDr
    uint8_t KDFcontext[sizeof(peerZid)+sizeof(zid)];
    int32_t kdfSize = sizeof(peerZid)+sizeof(zid);

    if (myRole == Responder) {
        memcpy(KDFcontext, peerZid, sizeof(peerZid));
        memcpy(KDFcontext+sizeof(peerZid), zid, sizeof(zid));
    }
    else {
        memcpy(KDFcontext, zid, sizeof(zid));
        memcpy(KDFcontext+sizeof(zid), peerZid, sizeof(peerZid));
    }
    
    KDF(zrtpSession, hashLength, (unsigned char*)zrtpTrustedMitm, strlen(zrtpTrustedMitm)+1,
        KDFcontext, kdfSize, SHA256_DIGEST_LENGTH * 8, pbxSecretTmpBuffer);

    pbxSecretTmp = pbxSecretTmpBuffer;  // set pointer to buffer, signal PBX secret was computed
}
    

void ZRtp::computeSRTPKeys() {

    // allocate the maximum size, compute real size to use
    uint8_t KDFcontext[sizeof(peerZid)+sizeof(zid)+sizeof(messageHash)];
    int32_t kdfSize = sizeof(peerZid)+sizeof(zid)+hashLength;

    int32_t keyLen = cipher->getKeylen() * 8;

    if (myRole == Responder) {
        memcpy(KDFcontext, peerZid, sizeof(peerZid));
        memcpy(KDFcontext+sizeof(peerZid), zid, sizeof(zid));
    }
    else {
        memcpy(KDFcontext, zid, sizeof(zid));
        memcpy(KDFcontext+sizeof(zid), peerZid, sizeof(peerZid));
    }
    memcpy(KDFcontext+sizeof(zid)+sizeof(peerZid), messageHash, hashLength);

    // Inititiator key and salt
    KDF(s0, hashLength, (unsigned char*)iniMasterKey, strlen(iniMasterKey)+1,
        KDFcontext, kdfSize, keyLen, srtpKeyI);
    KDF(s0, hashLength, (unsigned char*)iniMasterSalt, strlen(iniMasterSalt)+1,
        KDFcontext, kdfSize, 112, srtpSaltI);

    // Responder key and salt
    KDF(s0, hashLength, (unsigned char*)respMasterKey, strlen(respMasterKey)+1,
        KDFcontext, kdfSize, keyLen, srtpKeyR);
    KDF(s0, hashLength, (unsigned char*)respMasterSalt, strlen(respMasterSalt)+1,
        KDFcontext, kdfSize, 112, srtpSaltR);

    // The HMAC keys for GoClear
    KDF(s0, hashLength, (unsigned char*)iniHmacKey, strlen(iniHmacKey)+1,
        KDFcontext, kdfSize, hashLength*8, hmacKeyI);

    KDF(s0, hashLength, (unsigned char*)respHmacKey, strlen(respHmacKey)+1,
        KDFcontext, kdfSize, hashLength*8, hmacKeyR);

    // The keys for Confirm messages
    KDF(s0, hashLength, (unsigned char*)iniZrtpKey, strlen(iniZrtpKey)+1,
        KDFcontext, kdfSize, keyLen, zrtpKeyI);
    KDF(s0, hashLength, (unsigned char*)respZrtpKey, strlen(respZrtpKey)+1,
        KDFcontext, kdfSize, keyLen, zrtpKeyR);

    if (!multiStream) {
        // Compute the new Retained Secret
        KDF(s0, hashLength, (unsigned char*)retainedSec, strlen(retainedSec)+1,
            KDFcontext, kdfSize, SHA256_DIGEST_LENGTH*8, newRs1);

        // Compute the ZRTP Session Key
        KDF(s0, hashLength, (unsigned char*)zrtpSessionKey, strlen(zrtpSessionKey)+1,
            KDFcontext, kdfSize, hashLength*8, zrtpSession);

        // perform SAS generation according to chapter 5.5 and 8.
        // we don't need a speciai sasValue filed. sasValue are the first 
        // (leftmost) 32 bits (4 bytes) of sasHash
        uint8_t sasBytes[4];
        KDF(s0, hashLength, (unsigned char*)sasString, strlen(sasString)+1,
            KDFcontext, kdfSize, SHA256_DIGEST_LENGTH*8, sasHash);

        // according to chapter 8 only the leftmost 20 bits of sasValue (aka
        //  sasHash) are used to create the character SAS string of type SAS 
        // base 32 (5 bits per character)
        sasBytes[0] = sasHash[0];
        sasBytes[1] = sasHash[1];
        sasBytes[2] = sasHash[2] & 0xf0;
        sasBytes[3] = 0;
        SAS = Base32(sasBytes, 20).getEncoded();
    }
    memset(KDFcontext, 0, sizeof(KDFcontext));
}

bool ZRtp::srtpSecretsReady(EnableSecurity part) {

    SrtpSecret_t sec;

    sec.symEncAlgorithm = cipher->getAlgoId();

    sec.keyInitiator = srtpKeyI;
    sec.initKeyLen = cipher->getKeylen() * 8;
    sec.saltInitiator = srtpSaltI;
    sec.initSaltLen = 112;

    sec.keyResponder = srtpKeyR;
    sec.respKeyLen = cipher->getKeylen() * 8;
    sec.saltResponder = srtpSaltR;
    sec.respSaltLen = 112;

    sec.authAlgorithm = authLength->getAlgoId();
    sec.srtpAuthTagLen = authLength->getKeylen();

    sec.sas = SAS;
    sec.role = myRole;

    return callback->srtpSecretsReady(&sec, part);
}


void ZRtp::setNegotiatedHash(AlgorithmEnum* hash) {
    switch (zrtpHashes.getOrdinal(*hash)) {
    case 0:
        hashLength = SHA256_DIGEST_LENGTH;
        hashFunction = sha256;
        hashListFunction = sha256;

        hmacFunction = hmac_sha256;
        hmacListFunction = hmac_sha256;

        createHashCtx = createSha256Context;
        closeHashCtx = closeSha256Context;
        hashCtxFunction = sha256Ctx;
        hashCtxListFunction = sha256Ctx;
        break;

    case 1:
        hashLength = SHA384_DIGEST_LENGTH;
        hashFunction = sha384;
        hashListFunction = sha384;

        hmacFunction = hmac_sha384;
        hmacListFunction = hmac_sha384;

        createHashCtx = createSha384Context;
        closeHashCtx = closeSha384Context;
        hashCtxFunction = sha384Ctx;
        hashCtxListFunction = sha384Ctx;
        break;
    }
}


void ZRtp::srtpSecretsOff(EnableSecurity part) {
    callback->srtpSecretsOff(part);
}

void ZRtp::SASVerified() {
    // Initialize a ZID record to get peer's retained secrets
    ZIDRecord zidRec(peerZid);
    ZIDFile *zid = ZIDFile::getInstance();

    zid->getRecord(&zidRec);
    zidRec.setSasVerified();
    zid->saveRecord(&zidRec);
}

void ZRtp::resetSASVerified() {
    // Initialize a ZID record to get peer's retained secrets
    ZIDRecord zidRec(peerZid);
    ZIDFile *zid = ZIDFile::getInstance();

    zid->getRecord(&zidRec);
    zidRec.resetSasVerified();
    zid->saveRecord(&zidRec);
}


void ZRtp::sendInfo(GnuZrtpCodes::MessageSeverity severity, int32_t subCode) {
    callback->sendInfo(severity, subCode);
}


void ZRtp::zrtpNegotiationFailed(GnuZrtpCodes::MessageSeverity severity, int32_t subCode) {
    callback->zrtpNegotiationFailed(severity, subCode);
}

void ZRtp::zrtpNotSuppOther() {
    callback->zrtpNotSuppOther();
}

void ZRtp::synchEnter() {
    callback->synchEnter();
}

void ZRtp::synchLeave() {
    callback->synchLeave();
}


int32_t ZRtp::sendPacketZRTP(ZrtpPacketBase *packet) {
    return ((packet == NULL) ? 0 :
            callback->sendDataZRTP(packet->getHeaderBase(), (packet->getLength() * 4) + 4));
}

int32_t ZRtp::activateTimer(int32_t tm) {
    return (callback->activateTimer(tm));
}

int32_t ZRtp::cancelTimer() {
    return (callback->cancelTimer());
}


void ZRtp::setAuxSecret(uint8_t* data, int32_t length) {
    if (length > 0) {
        auxSecret = new uint8_t[length];
        auxSecretLength = length;
        memcpy(auxSecret, data, length);
    }
}

void ZRtp::setPbxSecret(uint8_t* data, int32_t length) {
}

void ZRtp::setClientId(std::string id) {
    if (id.size() < CLIENT_ID_SIZE) {
	unsigned char tmp[CLIENT_ID_SIZE +1] = {' '};
	memcpy(tmp, id.c_str(), id.size());
	tmp[CLIENT_ID_SIZE] = 0;
        zrtpHello.setClientId(tmp);
    } else {
        zrtpHello.setClientId((unsigned char*)id.c_str());
    }

    int32_t len = zrtpHello.getLength() * ZRTP_WORD_SIZE;

    // Hello packet is ready now, compute its HMAC
    // (excluding the HMAC field (2*ZTP_WORD_SIZE)) and store in Hello
    // use the implicit hash function
    uint8_t hmac[IMPL_MAX_DIGEST_LENGTH];
    uint32_t macLen;
    hmacFunctionImpl(H2, HASH_IMAGE_SIZE, (uint8_t*)zrtpHello.getHeaderBase(), 
                len-(2*ZRTP_WORD_SIZE), hmac, &macLen);
    zrtpHello.setHMAC(hmac);

    // calculate hash over the final Hello packet, refer to chap 9.1 how to
    // use this hash in SIP/SDP.
    hashFunctionImpl((uint8_t*)zrtpHello.getHeaderBase(), len, helloHash);
}

void ZRtp::storeMsgTemp(ZrtpPacketBase* pkt) {
    int32_t length = pkt->getLength() * ZRTP_WORD_SIZE;
    memset(tempMsgBuffer, 0, sizeof(tempMsgBuffer));
    memcpy(tempMsgBuffer, (uint8_t*)pkt->getHeaderBase(), length);
    lengthOfMsgData = length;
}

bool ZRtp::checkMsgHmac(uint8_t* key) {
    uint8_t hmac[IMPL_MAX_DIGEST_LENGTH];
    uint32_t macLen;
    int32_t len = lengthOfMsgData-(HMAC_SIZE);  // compute HMAC, but exlude the stored HMAC :-)

    // Use the implicit hash function
    hmacFunctionImpl(key, HASH_IMAGE_SIZE, tempMsgBuffer, len, hmac, &macLen);
    return (memcmp(hmac, tempMsgBuffer+len, (HMAC_SIZE)) == 0 ? true : false);
}


std::string ZRtp::getHelloHash() {
    std::ostringstream stm;

    uint8_t* hp = helloHash;

    stm << zrtpVersion;
    stm << " ";
    stm.fill('0');
    stm << hex;
    for (int i = 0; i < hashLengthImpl; i++) {
        stm.width(2);
        stm << static_cast<uint32_t>(*hp++);
    }
    return stm.str();
}

std::string ZRtp::getMultiStrParams() {

    // the string will hold binary data - it's opaque to the application
    std::string str("");
    char tmp[MAX_DIGEST_LENGTH + 1 + 1 + 1]; // hash length + cipher + authLength + hash

    if (inState(SecureState) && !multiStream) {
        // construct array that holds zrtpSession, cipher type, auth-length, and hash type
        tmp[0] = zrtpHashes.getOrdinal(*hash);
        tmp[1] = zrtpAuthLengths.getOrdinal(*authLength);
        tmp[2] = zrtpSymCiphers.getOrdinal(*cipher);
        memcpy(tmp+3, zrtpSession, hashLength);
        str.assign(tmp, hashLength + 1 + 1 + 1); // set chars (bytes) to the string
    }
    return str;
}

void ZRtp::setMultiStrParams(std::string parameters) {

    char tmp[MAX_DIGEST_LENGTH + 1 + 1 + 1]; // max. hash length + cipher + authLength + hash

    // First get negotiated hash from parameters, set algorithms and length
    int i = parameters.at(0) & 0xff;
    hash = &zrtpHashes.getByOrdinal(i);
    setNegotiatedHash(hash);           // sets hashlength

    // use string.copy(buffer, num, start=0) to retrieve chars (bytes) from the string
    parameters.copy(tmp, hashLength + 1 + 1 + 1, 0);

    i = tmp[1] & 0xff;
    authLength = &zrtpAuthLengths.getByOrdinal(i);
    i = tmp[2] & 0xff;
    cipher = &zrtpSymCiphers.getByOrdinal(i);
    memcpy(zrtpSession, tmp+3, hashLength);

    // after setting zrtpSession, cipher, and auth-length set multi-stream to true
    multiStream = true;
    stateEngine->setMultiStream(true);
}

bool ZRtp::isMultiStream() {
    return multiStream;
}

bool ZRtp::isMultiStreamAvailable() {
    return multiStreamAvailable;
}

void ZRtp::acceptEnrollment(bool accepted) {
    if (!accepted) {
        callback->zrtpInformEnrollment(EnrollmentCanceled);
        return;
    }
    // Get peer's zid record to store the pbx (MitM) secret
    // Initialize a ZID record to get peer's retained secrets
    ZIDRecord zidRec(peerZid);
    ZIDFile* zid = ZIDFile::getInstance();
    zid->getRecord(&zidRec);

    if (pbxSecretTmp != NULL) {
        zidRec.setMiTMData(pbxSecretTmp);
        callback->zrtpInformEnrollment(EnrollmentOk);
    }
    else {
        callback->zrtpInformEnrollment(EnrollmentFailed);
        return;
    }
    zid->saveRecord(&zidRec);
    return;
}

bool ZRtp::setSignatureData(uint8_t* data, int32_t length) {
    return false;
}

void ZRtp::conf2AckSecure() {
    Event_t ev;

    ev.type = ZrtpPacket;
    ev.packet = (uint8_t*)&zrtpConf2Ack;

    if (stateEngine != NULL) {
        stateEngine->processEvent(&ev);
    }
}

int32_t ZRtp::getSignatureData(uint8_t* data) {
    return 0;
}

int32_t ZRtp::getSignatureLength() {
    return 0;
}

int32_t ZRtp::compareCommit(ZrtpPacketCommit *commit) {
    // TODO: enhance to compare according to rules defined in chapter 4.2
    int32_t len = 0;
    len = !multiStream ? HVI_SIZE : (4 * ZRTP_WORD_SIZE);
    return (memcmp(hvi, commit->getHvi(), len)); 
}

bool ZRtp::isEnrollmentMode() {
    return enrollmentMode;
}

void ZRtp::setEnrollmentMode(bool enrollmentMode) {
    this->enrollmentMode = enrollmentMode;
}

bool ZRtp::sendSASRelayPacket(uint8_t* sh, std::string render) {

    uint8_t confMac[MAX_DIGEST_LENGTH];
    uint32_t macLen;
    uint8_t* hkey, *ekey;
    
    // If we are responder then the PBX used it's Initiator keys
    if (myRole == Responder) {
        hkey = hmacKeyR;
        ekey = zrtpKeyR;
    }
    else {
        hkey = hmacKeyI;
        ekey = zrtpKeyI;
    }
    // Prepare IV data that we will use during confirm packet encryption.
    randomZRTP(randomIV, sizeof(randomIV));
    zrtpSasRelay.setIv(randomIV);
    zrtpSasRelay.setTrustedSas(sh);
    zrtpSasRelay.setSas((uint8_t*)render.c_str());

    // Encrypt and HMAC with Initiator's key - we are Initiator here
    int16_t hmlen = (zrtpSasRelay.getLength() - 9) * ZRTP_WORD_SIZE;
    cipher->getEncrypt()(ekey, cipher->getKeylen(), randomIV,
                         (uint8_t*)zrtpSasRelay.getFiller(), hmlen);
    // Use negotiated HMAC (hash)
    hmacFunction(hkey, hashLength,
                 (unsigned char*)zrtpSasRelay.getFiller(),
                 hmlen, confMac, &macLen);

    zrtpSasRelay.setHmac(confMac);

    stateEngine->sendSASRelay(&zrtpSasRelay);
    return true;
}

std::string ZRtp::getSasType() {
    std::string sasT(sasType->getName());
    return sasT;
}
 
uint8_t* ZRtp::getSasHash() {
    return sasHash;
}

int32_t ZRtp::getPeerZid(uint8_t* data) {
    memcpy(data, peerZid, IDENTIFIER_LEN);
    return IDENTIFIER_LEN;
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-default-style: ellemtel
 * c-basic-offset: 4
 * End:
 */

