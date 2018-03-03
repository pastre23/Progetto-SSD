#!/bin/bash

# This script generates a self-signed certificate for use by the ESP8266
# Replace your-name-here with somethine appropriate before running and use
# the generated .H files in your code as follows:
#
#      static const uint8_t rsakey[]  ICACHE_RODATA_ATTR = {
#        #include "key.h"
#      };
#
#      static const uint8_t x509[] ICACHE_RODATA_ATTR = {
#        #include "x509.h"
#      };
#
#      ....
#      WiFiServerSecure server(443);
#      server.setServerKeyAndCert_P(rsakey, sizeof(rsakey), x509, sizeof(x509));
#      ....

# 1024 or 512.  512 saves memory...
BITS=512
C=$PWD
pushd /tmp


#CREATION CA ROOT

#1) Generation of a RSA Private Key with keysize 512 bit and encrypted with -des3
openssl genrsa -out PasqualeCA_key.pem $BITS -des3 -passout dattebayo

#2) Geration of a self-signed certificate
openssl req -x509 -key PasqualeCA_key.pem -new -days 365 -out PasqualeCA_x509cert.pem -sha256

#3) Conversion of certificate in .der (to store on EEPROM of Arduino)
openssl x509 -in PasqualeCA_x509cert.pem -outform DER -out PasqualeCA_x509cert.cer



#Creation Server Certificate
cat > certs.conf <<EOF
[ req ]
distinguished_name = req_distinguished_name
prompt = no

[ req_distinguished_name ]
O = ServerNodeMCU
CN = 172.20.10.3
EOF

#1) Generation of a RSA Private Key with keysize 512 bit and encrypted with -des3
openssl genrsa -out ServerNodeMCU_key.pem $BITS -des3 -passout hokage

#2) Conversion of RSA key in .der (to store on EEPROM of Arduino)
openssl rsa -in ServerNodeMCU_key.pem -out ServerNodeMCU_key -outform DER

#3) Request for a digital certificate
openssl req -out ServerNodeMCU_x509cert.req -key ServerNodeMCU_key.pem -new -config certs.conf

#4) Request to CA to sign the request and generate a digital certificate for Server
openssl x509 -req -in ServerNodeMCU_x509cert.req  -out ServerNodeMCU_x509cert.pem -sha256 -CAcreateserial
-days 5000 -CA PasqualeCA_x509cert.pem -CAkey PasqualeCA_key.pem

#5) Conversion of certificate in .der (to store on EEPROM of Arduino)
openssl x509 -in ServerNodeMCU_x509cert.pem -outform DER -out ServerNodeMCU_x509cert.cer #conversion

xxd -i ServerNodeMCU_key       | sed 's/.*{//' | sed 's/\};//' | sed 's/unsigned.*//' > "$C/serverKey.h"
xxd -i ServerNodeMCU_x509cert.cer  | sed 's/.*{//' | sed 's/\};//' | sed 's/unsigned.*//' > "$C/serverCert.h"
xxd -i PasqualeCA_x509cert.cer     | sed 's/.*{//' | sed 's/\};//' | sed 's/unsigned.*//' > "$C/caCert.h"


popd
