/*
 * Copyright (c) 2015 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __SECURITY_H__
#define __SECURITY_H__
 
#include <inttypes.h>
 
#define MBED_DOMAIN "e87f102c-9c71-4a53-92e4-66c45d1d3f0f"
#define MBED_ENDPOINT_NAME "a18fcea5-0752-43a7-995e-e3e588b3c787"
 
const uint8_t SERVER_CERT[] = "-----BEGIN CERTIFICATE-----\r\n"
"MIIBmDCCAT6gAwIBAgIEVUCA0jAKBggqhkjOPQQDAjBLMQswCQYDVQQGEwJGSTEN\r\n"
"MAsGA1UEBwwET3VsdTEMMAoGA1UECgwDQVJNMQwwCgYDVQQLDANJb1QxETAPBgNV\r\n"
"BAMMCEFSTSBtYmVkMB4XDTE1MDQyOTA2NTc0OFoXDTE4MDQyOTA2NTc0OFowSzEL\r\n"
"MAkGA1UEBhMCRkkxDTALBgNVBAcMBE91bHUxDDAKBgNVBAoMA0FSTTEMMAoGA1UE\r\n"
"CwwDSW9UMREwDwYDVQQDDAhBUk0gbWJlZDBZMBMGByqGSM49AgEGCCqGSM49AwEH\r\n"
"A0IABLuAyLSk0mA3awgFR5mw2RHth47tRUO44q/RdzFZnLsAsd18Esxd5LCpcT9w\r\n"
"0tvNfBv4xJxGw0wcYrPDDb8/rjujEDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0E\r\n"
"AwIDSAAwRQIhAPAonEAkwixlJiyYRQQWpXtkMZax+VlEiS201BG0PpAzAiBh2RsD\r\n"
"NxLKWwf4O7D6JasGBYf9+ZLwl0iaRjTjytO+Kw==\r\n"
"-----END CERTIFICATE-----\r\n";
 
const uint8_t CERT[] = "-----BEGIN CERTIFICATE-----\r\n"
"MIIBzzCCAXOgAwIBAgIEcdXxYjAMBggqhkjOPQQDAgUAMDkxCzAJBgNVBAYTAkZ\r\n"
"JMQwwCgYDVQQKDANBUk0xHDAaBgNVBAMME21iZWQtY29ubmVjdG9yLTIwMTgwHh\r\n"
"cNMTcwNTAyMTQyNTI2WhcNMTgxMjMxMDYwMDAwWjCBoTFSMFAGA1UEAxNJZTg3Z\r\n"
"jEwMmMtOWM3MS00YTUzLTkyZTQtNjZjNDVkMWQzZjBmL2ExOGZjZWE1LTA3NTIt\r\n"
"NDNhNy05OTVlLWUzZTU4OGIzYzc4NzEMMAoGA1UECxMDQVJNMRIwEAYDVQQKEwl\r\n"
"tYmVkIHVzZXIxDTALBgNVBAcTBE91bHUxDTALBgNVBAgTBE91bHUxCzAJBgNVBA\r\n"
"YTAkZJMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAExIOdKkHrxRGFgYFeKVta8\r\n"
"oZ/kihQwrIVd4WXTelkKm83o706fKfbUSSRcdhqArjI1Tz8N7oAvnSylwdrxHQS\r\n"
"rDAMBggqhkjOPQQDAgUAA0gAMEUCIQCDhKuhQZtlolySGQQXjSHjKgTANm4ZlfD\r\n"
"8f/WgQ0P8sgIgcOu4ogjcZiUvWBNZCoJZmqo8Q+UsTNveVhzmEl9ILlI=\r\n"
"-----END CERTIFICATE-----\r\n";
 
const uint8_t KEY[] = "-----BEGIN PRIVATE KEY-----\r\n"
"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgormkaWYL7xLLfRj6\r\n"
"wegjot2zZQtSl4v/eVh7aA+5w/ahRANCAATEg50qQevFEYWBgV4pW1ryhn+SKFDC\r\n"
"shV3hZdN6WQqbzejvTp8p9tRJJFx2GoCuMjVPPw3ugC+dLKXB2vEdBKs\r\n"
"-----END PRIVATE KEY-----\r\n";
 
#endif //__SECURITY_H__
