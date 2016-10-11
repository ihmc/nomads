The OpenSSL libraries in this directory have been built using openssl-1.0.0d
They have been built using Visual Studio 2008 on a 64-bit Windows Vista system

When using the include files and the pre-compiled libraries, it may be important to make sure that
the code using the include files uses the same set of preprocessor definitions as when the code was
compiled into the library

Below are the preprocessor definitions that were used when building the libraries

For Release (Compiled with /MD) and Static Release (Compiled with /MT)
-DOPENSSL_THREADS -DDSO_WIN32 -DOPENSSL_SYSNAME_WIN32 -DWIN32_LEAN_AND_MEAN -DL_ENDIAN -D_CRT_SECURE_NO_DEPRECATE -DOPENSSL_BN_ASM_PART_WORDS -DOPENSSL_IA32_SSE2 -DOPENSSL_BN_ASM_MONT -DSHA1_ASM -DSHA256_ASM -DSHA512_ASM -DMD5_ASM -DRMD160_ASM -DAES_ASM -DWHIRLPOOL_ASM -DOPENSSL_NO_RC5 -DOPENSSL_NO_MD2 -DOPENSSL_NO_KRB5 -DOPENSSL_NO_JPAKE -DOPENSSL_NO_DYNAMIC_ENGINE

For Debug (Compiled with /MDd) and Static Debug (Compiled with /MTd)
-DDEBUG -D_DEBUG -DOPENSSL_THREADS -DDSO_WIN32 -DOPENSSL_SYSNAME_WIN32 -DWIN32_LEAN_AND_MEAN -DL_ENDIAN -D_CRT_SECURE_NO_DEPRECATE -DOPENSSL_BN_ASM_PART_WORDS -DOPENSSL_IA32_SSE2 -DOPENSSL_BN_ASM_MONT -DSHA1_ASM -DSHA256_ASM -DSHA512_ASM -DMD5_ASM -DRMD160_ASM -DAES_ASM -DWHIRLPOOL_ASM -DOPENSSL_NO_RC5 -DOPENSSL_NO_MD2 -DOPENSSL_NO_KRB5 -DOPENSSL_NO_JPAKE -DOPENSSL_NO_DYNAMIC_ENGINE