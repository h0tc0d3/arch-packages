/* ecc-curves.c  -  Elliptic Curve parameter mangement
 * Copyright (C) 2007, 2008, 2010, 2011 Free Software Foundation, Inc.
 * Copyright (C) 2013 g10 Code GmbH
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "g10lib.h"
#include "mpi.h"
#include "mpi-internal.h"
#include "cipher.h"
#include "context.h"
#include "ec-context.h"
#include "pubkey-internal.h"
#include "ecc-common.h"


static gpg_err_code_t
point_from_keyparam (gcry_mpi_point_t *r_a,
                     gcry_sexp_t keyparam, const char *name, mpi_ec_t ec);

/* This tables defines aliases for curve names.  */
static const struct
{
  const char *name;  /* Our name.  */
  const char *other; /* Other name. */
} curve_aliases[] =
  {
    { "Ed25519",    "1.3.6.1.4.1.11591.15.1" }, /* OpenPGP */
    { "Ed25519",    "1.3.101.112" },         /* rfc8410 */

    { "Curve25519", "1.3.6.1.4.1.3029.1.5.1" }, /* OpenPGP */
    { "Curve25519", "1.3.101.110" },         /* rfc8410 */
    { "Curve25519", "X25519" },              /* rfc8410 */

    { "Ed448",      "1.3.101.113" },         /* rfc8410 */
    { "X448",       "1.3.101.111" },         /* rfc8410 */

    { "NIST P-224", "secp224r1" },
    { "NIST P-224", "1.3.132.0.33" },        /* SECP OID.  */
    { "NIST P-224", "nistp224"   },          /* rfc5656.  */

    { "NIST P-256", "1.2.840.10045.3.1.7" }, /* From NIST SP 800-78-1.  */
    { "NIST P-256", "prime256v1" },
    { "NIST P-256", "secp256r1"  },
    { "NIST P-256", "nistp256"   },          /* rfc5656.  */

    { "NIST P-384", "secp384r1" },
    { "NIST P-384", "1.3.132.0.34" },
    { "NIST P-384", "nistp384"   },          /* rfc5656.  */

    { "NIST P-521", "secp521r1" },
    { "NIST P-521", "1.3.132.0.35" },
    { "NIST P-521", "nistp521"   },          /* rfc5656.  */

    { "GOST2001-test", "1.2.643.2.2.35.0" },
    { "GOST2001-CryptoPro-A", "1.2.643.2.2.35.1" },
    { "GOST2001-CryptoPro-B", "1.2.643.2.2.35.2" },
    { "GOST2001-CryptoPro-C", "1.2.643.2.2.35.3" },
    { "GOST2001-CryptoPro-A", "GOST2001-CryptoPro-XchA" },
    { "GOST2001-CryptoPro-C", "GOST2001-CryptoPro-XchB" },
    { "GOST2001-CryptoPro-A", "1.2.643.2.2.36.0" },
    { "GOST2001-CryptoPro-C", "1.2.643.2.2.36.1" },

    { "GOST2012-256-tc26-A", "1.2.643.7.1.2.1.1.1" },
    { "GOST2001-CryptoPro-A", "1.2.643.7.1.2.1.1.2" },
    { "GOST2001-CryptoPro-A", "GOST2012-256-tc26-B" },
    { "GOST2001-CryptoPro-B", "1.2.643.7.1.2.1.1.3" },
    { "GOST2001-CryptoPro-B", "GOST2012-256-tc26-C" },
    { "GOST2001-CryptoPro-C", "1.2.643.7.1.2.1.1.4" },
    { "GOST2001-CryptoPro-C", "GOST2012-256-tc26-D" },

    { "GOST2012-512-test", "GOST2012-test" },
    { "GOST2012-512-test", "1.2.643.7.1.2.1.2.0" },
    { "GOST2012-512-tc26-A", "GOST2012-tc26-A" },
    { "GOST2012-512-tc26-B", "GOST2012-tc26-B" },
    { "GOST2012-512-tc26-A", "1.2.643.7.1.2.1.2.1" },
    { "GOST2012-512-tc26-B", "1.2.643.7.1.2.1.2.2" },
    { "GOST2012-512-tc26-C", "1.2.643.7.1.2.1.2.3" },

    { "secp256k1", "1.3.132.0.10" },

    { "sm2p256v1", "1.2.156.10197.1.301" },

    { NULL, NULL}
  };


typedef struct
{
  const char *desc;           /* Description of the curve.  */
  unsigned int nbits;         /* Number of bits.  */
  unsigned int fips:1;        /* True if this is a FIPS140-2 approved curve. */

  /* The model describing this curve.  This is mainly used to select
     the group equation. */
  enum gcry_mpi_ec_models model;

  /* The actual ECC dialect used.  This is used for curve specific
     optimizations and to select encodings etc. */
  enum ecc_dialects dialect;

  const char *p;              /* The prime defining the field.  */
  const char *a, *b;          /* The coefficients.  For Twisted Edwards
                                 Curves b is used for d.  For Montgomery
                                 Curves (a,b) has ((A-2)/4,B^-1).  */
  const char *n;              /* The order of the base point.  */
  const char *g_x, *g_y;      /* Base point.  */
  unsigned int h;             /* Cofactor.  */
} ecc_domain_parms_t;


/* This static table defines all available curves.  */
static const ecc_domain_parms_t domain_parms[] =
  {
    {
      /* (-x^2 + y^2 = 1 + dx^2y^2) */
      "Ed25519", 255, 0,
      MPI_EC_EDWARDS, ECC_DIALECT_ED25519,
      "0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFED",
      "-0x01",
      "-0x2DFC9311D490018C7338BF8688861767FF8FF5B2BEBE27548A14B235ECA6874A",
      "0x1000000000000000000000000000000014DEF9DEA2F79CD65812631A5CF5D3ED",
      "0x216936D3CD6E53FEC0A4E231FDD6DC5C692CC7609525A7B2C9562D608F25D51A",
      "0x6666666666666666666666666666666666666666666666666666666666666658",
      8
    },
    {
      /* (y^2 = x^3 + 486662*x^2 + x) */
      "Curve25519", 255, 0,
      MPI_EC_MONTGOMERY, ECC_DIALECT_STANDARD,
      "0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFED",
      "0x01DB41",
      "0x01",
      "0x1000000000000000000000000000000014DEF9DEA2F79CD65812631A5CF5D3ED",
      "0x0000000000000000000000000000000000000000000000000000000000000009",
      "0x20AE19A1B8A086B4E01EDD2C7748D14C923D4D7E6D7C61B229E9C5A27ECED3D9",
      8
      /* Note: As per RFC-7748 errata eid4730 the g_y value should be
       * "0x5F51E65E475F794B1FE122D388B72EB36DC2B28192839E4DD6163A5D81312C14"
       * but that breaks the keygrip.  The new value is recovered in
       * the function _gcry_ecc_fill_in_curve.  See bug #4712.
       */
    },
    {
      /* (x^2 + y^2 = 1 + dx^2y^2) */
      "Ed448", 448, 0,
      MPI_EC_EDWARDS, ECC_DIALECT_SAFECURVE,
      "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE"
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
      "0x01",
      "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE"
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF6756",
      "0x3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
      "7CCA23E9C44EDB49AED63690216CC2728DC58F552378C292AB5844F3",
      "0x4F1970C66BED0DED221D15A622BF36DA9E146570470F1767EA6DE324"
      "A3D3A46412AE1AF72AB66511433B80E18B00938E2626A82BC70CC05E",
      "0x693F46716EB6BC248876203756C9C7624BEA73736CA3984087789C1E"
      "05A0C2D73AD3FF1CE67C39C4FDBD132C4ED7C8AD9808795BF230FA14",
      4,
    },
    {
      /* (y^2 = x^3 + 156326*x^2 + x) */
      "X448", 448, 0,
      MPI_EC_MONTGOMERY, ECC_DIALECT_SAFECURVE,
      "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE"
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
      "0x98A9",
      "0x01",
      "0x3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
      "7CCA23E9C44EDB49AED63690216CC2728DC58F552378C292AB5844F3",
      "0x00000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000005",
      "0x7D235D1295F5B1F66C98AB6E58326FCECBAE5D34F55545D060F75DC2"
      "8DF3F6EDB8027E2346430D211312C4B150677AF76FD7223D457B5B1A",
      4,
    },
    {
      "NIST P-224", 224, 1,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0xffffffffffffffffffffffffffffffff000000000000000000000001",
      "0xfffffffffffffffffffffffffffffffefffffffffffffffffffffffe",
      "0xb4050a850c04b3abf54132565044b0b7d7bfd8ba270b39432355ffb4",
      "0xffffffffffffffffffffffffffff16a2e0b8f03e13dd29455c5c2a3d" ,

      "0xb70e0cbd6bb4bf7f321390b94a03c1d356c21122343280d6115c1d21",
      "0xbd376388b5f723fb4c22dfe6cd4375a05a07476444d5819985007e34",
      1
    },
    {
      "NIST P-256", 256, 1,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0xffffffff00000001000000000000000000000000ffffffffffffffffffffffff",
      "0xffffffff00000001000000000000000000000000fffffffffffffffffffffffc",
      "0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b",
      "0xffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551",

      "0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296",
      "0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5",
      1
    },
    {
      "NIST P-384", 384, 1,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe"
      "ffffffff0000000000000000ffffffff",
      "0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe"
      "ffffffff0000000000000000fffffffc",
      "0xb3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875a"
      "c656398d8a2ed19d2a85c8edd3ec2aef",
      "0xffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf"
      "581a0db248b0a77aecec196accc52973",

      "0xaa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a38"
      "5502f25dbf55296c3a545e3872760ab7",
      "0x3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c0"
      "0a60b1ce1d7e819d7a431d7c90ea0e5f",
      1
    },
    {
      "NIST P-521", 521, 1,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0x01ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
      "0x01ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffc",
      "0x051953eb9618e1c9a1f929a21a0b68540eea2da725b99b315f3b8b489918ef10"
      "9e156193951ec7e937b1652c0bd3bb1bf073573df883d2c34f1ef451fd46b503f00",
      "0x01ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "fffa51868783bf2f966b7fcc0148f709a5d03bb5c9b8899c47aebb6fb71e91386409",

      "0x00c6858e06b70404e9cd9e3ecb662395b4429c648139053fb521f828af606b4d"
      "3dbaa14b5e77efe75928fe1dc127a2ffa8de3348b3c1856a429bf97e7e31c2e5bd66",
      "0x011839296a789a3bc0045c8a5fb42c7d1bd998f54449579b446817afbd17273e"
      "662c97ee72995ef42640c550b9013fad0761353c7086a272c24088be94769fd16650",
      1
    },

    {
      "GOST2001-test", 256, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0x8000000000000000000000000000000000000000000000000000000000000431",
      "0x0000000000000000000000000000000000000000000000000000000000000007",
      "0x5fbff498aa938ce739b8e022fbafef40563f6e6a3472fc2a514c0ce9dae23b7e",
      "0x8000000000000000000000000000000150fe8a1892976154c59cfc193accf5b3",

      "0x0000000000000000000000000000000000000000000000000000000000000002",
      "0x08e2a8a0e65147d4bd6316030e16d19c85c97f0a9ca267122b96abbcea7e8fc8",
      1
    },
    {
      "GOST2001-CryptoPro-A", 256, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffd97",
      "0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffd94",
      "0x00000000000000000000000000000000000000000000000000000000000000a6",
      "0xffffffffffffffffffffffffffffffff6c611070995ad10045841b09b761b893",
      "0x0000000000000000000000000000000000000000000000000000000000000001",
      "0x8d91e471e0989cda27df505a453f2b7635294f2ddf23e3b122acc99c9e9f1e14",
      1
    },
    {
      "GOST2001-CryptoPro-B", 256, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0x8000000000000000000000000000000000000000000000000000000000000c99",
      "0x8000000000000000000000000000000000000000000000000000000000000c96",
      "0x3e1af419a269a5f866a7d3c25c3df80ae979259373ff2b182f49d4ce7e1bbc8b",
      "0x800000000000000000000000000000015f700cfff1a624e5e497161bcc8a198f",
      "0x0000000000000000000000000000000000000000000000000000000000000001",
      "0x3fa8124359f96680b83d1c3eb2c070e5c545c9858d03ecfb744bf8d717717efc",
      1
    },
    {
      "GOST2001-CryptoPro-C", 256, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0x9b9f605f5a858107ab1ec85e6b41c8aacf846e86789051d37998f7b9022d759b",
      "0x9b9f605f5a858107ab1ec85e6b41c8aacf846e86789051d37998f7b9022d7598",
      "0x000000000000000000000000000000000000000000000000000000000000805a",
      "0x9b9f605f5a858107ab1ec85e6b41c8aa582ca3511eddfb74f02f3a6598980bb9",
      "0x0000000000000000000000000000000000000000000000000000000000000000",
      "0x41ece55743711a8c3cbf3783cd08c0ee4d4dc440d4641a8f366e550dfdb3bb67",
      1
    },
    {
      "GOST2012-256-A", 256, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffd97",
      "0xc2173f1513981673af4892c23035a27ce25e2013bf95aa33b22c656f277e7335",
      "0x295f9bae7428ed9ccc20e7c359a9d41a22fccd9108e17bf7ba9337a6f8ae9513",
      "0x400000000000000000000000000000000fd8cddfc87b6635c115af556c360c67",
      "0x91e38443a5e82c0d880923425712b2bb658b9196932e02c78b2582fe742daa28",
      "0x32879423ab1a0375895786c4bb46e9565fde0b5344766740af268adb32322e5c",
      4
    },
    {
      "GOST2012-512-test", 511, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0x4531acd1fe0023c7550d267b6b2fee80922b14b2ffb90f04d4eb7c09b5d2d15d"
      "f1d852741af4704a0458047e80e4546d35b8336fac224dd81664bbf528be6373",
      "0x0000000000000000000000000000000000000000000000000000000000000007",
      "0x1cff0806a31116da29d8cfa54e57eb748bc5f377e49400fdd788b649eca1ac4"
      "361834013b2ad7322480a89ca58e0cf74bc9e540c2add6897fad0a3084f302adc",
      "0x4531acd1fe0023c7550d267b6b2fee80922b14b2ffb90f04d4eb7c09b5d2d15d"
      "a82f2d7ecb1dbac719905c5eecc423f1d86e25edbe23c595d644aaf187e6e6df",

      "0x24d19cc64572ee30f396bf6ebbfd7a6c5213b3b3d7057cc825f91093a68cd762"
      "fd60611262cd838dc6b60aa7eee804e28bc849977fac33b4b530f1b120248a9a",
      "0x2bb312a43bd2ce6e0d020613c857acddcfbf061e91e5f2c3f32447c259f39b2"
      "c83ab156d77f1496bf7eb3351e1ee4e43dc1a18b91b24640b6dbb92cb1add371e",
      1
    },
    {
      "GOST2012-512-tc26-A", 512, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffdc7",
      "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffdc4",
      "0xe8c2505dedfc86ddc1bd0b2b6667f1da34b82574761cb0e879bd081cfd0b6265"
        "ee3cb090f30d27614cb4574010da90dd862ef9d4ebee4761503190785a71c760",
      "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
        "27e69532f48d89116ff22b8d4e0560609b4b38abfad2b85dcacdb1411f10b275",
      "0x0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000003",
      "0x7503cfe87a836ae3a61b8816e25450e6ce5e1c93acf1abc1778064fdcbefa921"
        "df1626be4fd036e93d75e6a50e3a41e98028fe5fc235f5b889a589cb5215f2a4",
      1
    },
    {
      "GOST2012-512-tc26-B", 512, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0x8000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000006f",
      "0x8000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000006c",
      "0x687d1b459dc841457e3e06cf6f5e2517b97c7d614af138bcbf85dc806c4b289f"
        "3e965d2db1416d217f8b276fad1ab69c50f78bee1fa3106efb8ccbc7c5140116",
      "0x8000000000000000000000000000000000000000000000000000000000000001"
        "49a1ec142565a545acfdb77bd9d40cfa8b996712101bea0ec6346c54374f25bd",
      "0x0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000002",
      "0x1a8f7eda389b094c2c071e3647a8940f3c123b697578c213be6dd9e6c8ec7335"
        "dcb228fd1edf4a39152cbcaaf8c0398828041055f94ceeec7e21340780fe41bd",
      1
    },
    {
      "GOST2012-512-tc26-C", 512, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffdc7",
      "0xdc9203e514a721875485a529d2c722fb187bc8980eb866644de41c68e1430645"
        "46e861c0e2c9edd92ade71f46fcf50ff2ad97f951fda9f2a2eb6546f39689bd3",
      "0xb4c4ee28cebc6c2c8ac12952cf37f16ac7efb6a9f69f4b57ffda2e4f0de5ade0"
        "38cbc2fff719d2c18de0284b8bfef3b52b8cc7a5f5bf0a3c8d2319a5312557e1",
      "0x3fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
        "c98cdba46506ab004c33a9ff5147502cc8eda9e7a769a12694623cef47f023ed",
      "0xe2e31edfc23de7bdebe241ce593ef5de2295b7a9cbaef021d385f7074cea043a"
        "a27272a7ae602bf2a7b9033db9ed3610c6fb85487eae97aac5bc7928c1950148",
      "0xf5ce40d95b5eb899abbccff5911cb8577939804d6527378b8c108c3d2090ff9be"
        "18e2d33e3021ed2ef32d85822423b6304f726aa854bae07d0396e9a9addc40f",
      4
    },

    {
      "secp256k1", 256, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F",
      "0x0000000000000000000000000000000000000000000000000000000000000000",
      "0x0000000000000000000000000000000000000000000000000000000000000007",
      "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141",
      "0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798",
      "0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8",
      1
    },

    {
      "sm2p256v1", 256, 0,
      MPI_EC_WEIERSTRASS, ECC_DIALECT_STANDARD,
      "0xfffffffeffffffffffffffffffffffffffffffff00000000ffffffffffffffff",
      "0xfffffffeffffffffffffffffffffffffffffffff00000000fffffffffffffffc",
      "0x28e9fa9e9d9f5e344d5a9e4bcf6509a7f39789f515ab8f92ddbcbd414d940e93",
      "0xfffffffeffffffffffffffffffffffff7203df6b21c6052b53bbf40939d54123",
      "0x32c4ae2c1f1981195f9904466a39c9948fe30bbff2660be1715a4589334c74c7",
      "0xbc3736a2f4f6779c59bdcee36b692153d0a9877cc62a474002df32e52139f0a0",
      1
    },

    { NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL }
  };




/* Return a copy of POINT.  */
static gcry_mpi_point_t
point_copy (gcry_mpi_point_t point)
{
  gcry_mpi_point_t newpoint;

  if (point)
    {
      newpoint = mpi_point_new (0);
      point_set (newpoint, point);
    }
  else
    newpoint = NULL;
  return newpoint;
}


/* Helper to scan a hex string. */
static gcry_mpi_t
scanval (const char *string)
{
  gpg_err_code_t rc;
  gcry_mpi_t val;

  rc = _gcry_mpi_scan (&val, GCRYMPI_FMT_HEX, string, 0, NULL);
  if (rc)
    log_fatal ("scanning ECC parameter failed: %s\n", gpg_strerror (rc));
  return val;
}


/* Return the index of the domain_parms table for a curve with NAME.
   Return -1 if not found.  */
static int
find_domain_parms_idx (const char *name)
{
  int idx, aliasno;

  /* First check our native curves.  */
  for (idx = 0; domain_parms[idx].desc; idx++)
    if (!strcmp (name, domain_parms[idx].desc))
      return idx;

  /* If not found consult the alias table.  */
  if (!domain_parms[idx].desc)
    {
      for (aliasno = 0; curve_aliases[aliasno].name; aliasno++)
        if (!strcmp (name, curve_aliases[aliasno].other))
          break;
      if (curve_aliases[aliasno].name)
        {
          for (idx = 0; domain_parms[idx].desc; idx++)
            if (!strcmp (curve_aliases[aliasno].name, domain_parms[idx].desc))
              return idx;
        }
    }

  return -1;
}


/* Generate the crypto system setup.  This function takes the NAME of
   a curve or the desired number of bits and stores at R_CURVE the
   parameters of the named curve or those of a suitable curve.  If
   R_NBITS is not NULL, the chosen number of bits is stored there.
   NULL may be given for R_CURVE, if the value is not required and for
   example only a quick test for availability is desired.  Note that
   the curve fields should be initialized to zero because fields which
   are not NULL are skipped.  */
gpg_err_code_t
_gcry_ecc_fill_in_curve (unsigned int nbits, const char *name,
                         elliptic_curve_t *curve, unsigned int *r_nbits)
{
  int idx;
  const char *resname = NULL; /* Set to a found curve name.  */

  if (name)
    idx = find_domain_parms_idx (name);
  else
    {
      for (idx = 0; domain_parms[idx].desc; idx++)
        if (nbits == domain_parms[idx].nbits
            && domain_parms[idx].model == MPI_EC_WEIERSTRASS)
          break;
      if (!domain_parms[idx].desc)
        idx = -1;
    }
  if (idx < 0)
    return GPG_ERR_UNKNOWN_CURVE;

  resname = domain_parms[idx].desc;

  /* In fips mode we only support NIST curves.  Note that it is
     possible to bypass this check by specifying the curve parameters
     directly.  */
  if (fips_mode () && !domain_parms[idx].fips )
    return GPG_ERR_NOT_SUPPORTED;

  switch (domain_parms[idx].model)
    {
    case MPI_EC_WEIERSTRASS:
    case MPI_EC_EDWARDS:
    case MPI_EC_MONTGOMERY:
      break;
    default:
      return GPG_ERR_BUG;
    }


  if (r_nbits)
    *r_nbits = domain_parms[idx].nbits;

  if (curve)
    {
      curve->model = domain_parms[idx].model;
      curve->dialect = domain_parms[idx].dialect;
      if (!curve->p)
        curve->p = scanval (domain_parms[idx].p);
      if (!curve->a)
        {
          curve->a = scanval (domain_parms[idx].a);
          if (curve->a->sign)
            {
              mpi_resize (curve->a, curve->p->nlimbs);
              _gcry_mpih_sub_n (curve->a->d, curve->p->d,
                                curve->a->d, curve->p->nlimbs);
              curve->a->nlimbs = curve->p->nlimbs;
              curve->a->sign = 0;
            }
        }
      if (!curve->b)
        {
          curve->b = scanval (domain_parms[idx].b);
          if (curve->b->sign)
            {
              mpi_resize (curve->b, curve->p->nlimbs);
              _gcry_mpih_sub_n (curve->b->d, curve->p->d,
                                curve->b->d, curve->p->nlimbs);
              curve->b->nlimbs = curve->p->nlimbs;
              curve->b->sign = 0;
            }
        }
      if (!curve->n)
        curve->n = scanval (domain_parms[idx].n);
      if (!curve->G.x)
        curve->G.x = scanval (domain_parms[idx].g_x);
      if (!curve->G.y)
        curve->G.y = scanval (domain_parms[idx].g_y);
      curve->h = domain_parms[idx].h;

      /*
       * In the constants of domain_parms, we defined Curve25519
       * domain parameters as the ones in RFC-7748 before the errata
       * (eid4730).  To keep the computation having exact same values,
       * we recover the new value of g_y, here.
       */
      if (!strcmp (resname, "Curve25519"))
        mpi_sub (curve->G.y, curve->p, curve->G.y);

      if (!curve->G.z)
        curve->G.z = mpi_alloc_set_ui (1);
      if (!curve->name)
        curve->name = resname;
    }

  return 0;
}


/* Give the name of the curve NAME, store the curve parameters into P,
   A, B, G, and N if they point to NULL value.  Note that G is
   returned in standard uncompressed format.  Also update MODEL and
   DIALECT if they are not NULL. */
gpg_err_code_t
_gcry_ecc_update_curve_param (const char *name,
                              enum gcry_mpi_ec_models *model,
                              enum ecc_dialects *dialect,
                              gcry_mpi_t *p, gcry_mpi_t *a, gcry_mpi_t *b,
                              gcry_mpi_t *g, gcry_mpi_t *n)
{
  int idx;

  idx = find_domain_parms_idx (name);
  if (idx < 0)
    return GPG_ERR_UNKNOWN_CURVE;

  if (g)
    {
      char *buf;
      size_t len;

      len = 4;
      len += strlen (domain_parms[idx].g_x+2);
      len += strlen (domain_parms[idx].g_y+2);
      len++;
      buf = xtrymalloc (len);
      if (!buf)
        return gpg_err_code_from_syserror ();
      strcpy (stpcpy (stpcpy (buf, "0x04"), domain_parms[idx].g_x+2),
              domain_parms[idx].g_y+2);
      _gcry_mpi_release (*g);
      *g = scanval (buf);
      xfree (buf);
    }
  if (model)
    *model = domain_parms[idx].model;
  if (dialect)
    *dialect = domain_parms[idx].dialect;
  if (p)
    {
      _gcry_mpi_release (*p);
      *p = scanval (domain_parms[idx].p);
    }
  if (a)
    {
      _gcry_mpi_release (*a);
      *a = scanval (domain_parms[idx].a);
    }
  if (b)
    {
      _gcry_mpi_release (*b);
      *b = scanval (domain_parms[idx].b);
    }
  if (n)
    {
      _gcry_mpi_release (*n);
      *n = scanval (domain_parms[idx].n);
    }
  return 0;
}


/* Return the name matching the parameters in PKEY.  This works only
   with curves described by the Weierstrass equation. */
const char *
_gcry_ecc_get_curve (gcry_sexp_t keyparms, int iterator, unsigned int *r_nbits)
{
  gpg_err_code_t rc;
  const char *result = NULL;
  elliptic_curve_t E;
  gcry_mpi_point_t G = NULL;
  gcry_mpi_t tmp = NULL;
  int idx;

  memset (&E, 0, sizeof E);

  if (r_nbits)
    *r_nbits = 0;

  if (!keyparms)
    {
      idx = iterator;
      if (idx >= 0 && idx < DIM (domain_parms))
        {
          result = domain_parms[idx].desc;
          if (r_nbits)
            *r_nbits = domain_parms[idx].nbits;
        }
      return result;
    }


  /*
   * Extract the curve parameters..
   */
  rc = gpg_err_code (sexp_extract_param (keyparms, NULL, "pabn",
                                         &E.p, &E.a, &E.b, &E.n, NULL));
  if (rc == GPG_ERR_NO_OBJ)
    {
      /* This might be the second use case of checking whether a
         specific curve given by name is supported.  */
      gcry_sexp_t l1;
      char *name;

      l1 = sexp_find_token (keyparms, "curve", 5);
      if (!l1)
        goto leave;  /* No curve name parameter.  */

      name = sexp_nth_string (l1, 1);
      sexp_release (l1);
      if (!name)
        goto leave;  /* Name missing or out of core. */

      idx = find_domain_parms_idx (name);
      xfree (name);
      if (idx >= 0)  /* Curve found.  */
        {
          result = domain_parms[idx].desc;
          if (r_nbits)
            *r_nbits = domain_parms[idx].nbits;
        }
      return result;
    }

  if (rc)
    goto leave;

  rc = point_from_keyparam (&G, keyparms, "g", NULL);
  if (rc)
    goto leave;

  _gcry_mpi_point_init (&E.G);
  _gcry_mpi_point_set (&E.G, G->x, G->y, G->z);

  for (idx = 0; domain_parms[idx].desc; idx++)
    {
      mpi_free (tmp);
      tmp = scanval (domain_parms[idx].p);
      if (mpi_cmp (tmp, E.p))
        continue;

      mpi_free (tmp);
      tmp = scanval (domain_parms[idx].a);
      if (tmp->sign)
        {
          if (!mpi_cmpabs (tmp, E.a))
            /* For backward compatibility to <= libgcrypt 1.8, we
               allow this match to support existing keys in SEXP.  */
            ;
          else
            {
              mpi_resize (tmp, E.p->nlimbs);
              _gcry_mpih_sub_n (tmp->d, E.p->d,
                                tmp->d, E.p->nlimbs);
              tmp->nlimbs = E.p->nlimbs;
              tmp->sign = 0;
              if (mpi_cmp (tmp, E.a))
                continue;
            }
        }
      else if (mpi_cmp (tmp, E.a))
        continue;

      mpi_free (tmp);
      tmp = scanval (domain_parms[idx].b);
      if (tmp->sign)
        {
          if (!mpi_cmpabs (tmp, E.b))
            /* Same for backward compatibility, see above.  */
            ;
          else
            {
              mpi_resize (tmp, E.p->nlimbs);
              _gcry_mpih_sub_n (tmp->d, E.p->d,
                                tmp->d, E.p->nlimbs);
              tmp->nlimbs = E.p->nlimbs;
              tmp->sign = 0;
              if (mpi_cmp (tmp, E.b))
                continue;
            }
        }
      else if (mpi_cmp (tmp, E.b))
        continue;

      mpi_free (tmp);
      tmp = scanval (domain_parms[idx].n);
      if (mpi_cmp (tmp, E.n))
        continue;

      mpi_free (tmp);
      tmp = scanval (domain_parms[idx].g_x);
      if (mpi_cmp (tmp, E.G.x))
        continue;

      mpi_free (tmp);
      tmp = scanval (domain_parms[idx].g_y);
      if (mpi_cmp (tmp, E.G.y))
        continue;

      result = domain_parms[idx].desc;
      if (r_nbits)
        *r_nbits = domain_parms[idx].nbits;
      break;
    }

 leave:
  _gcry_mpi_point_release (G);
  _gcry_mpi_release (tmp);
  _gcry_mpi_release (E.p);
  _gcry_mpi_release (E.a);
  _gcry_mpi_release (E.b);
  _gcry_mpi_point_free_parts (&E.G);
  _gcry_mpi_release (E.n);
  return result;
}


/* Helper to extract an MPI from key parameters.  */
static gpg_err_code_t
mpi_from_keyparam (gcry_mpi_t *r_a, gcry_sexp_t keyparam, const char *name,
                   int opaque)
{
  gcry_err_code_t ec = 0;
  gcry_sexp_t l1;

  l1 = sexp_find_token (keyparam, name, 0);
  if (l1)
    {
      *r_a = sexp_nth_mpi (l1, 1, opaque? GCRYMPI_FMT_OPAQUE : GCRYMPI_FMT_USG);
      sexp_release (l1);
      if (!*r_a)
        ec = GPG_ERR_INV_OBJ;
    }
  return ec;
}

/* Helper to extract a point from key parameters.  If no parameter
   with NAME is found, the functions tries to find a non-encoded point
   by appending ".x", ".y" and ".z" to NAME.  ".z" is in this case
   optional and defaults to 1.  EC is the context which at this point
   may not be fully initialized. */
static gpg_err_code_t
point_from_keyparam (gcry_mpi_point_t *r_a,
                     gcry_sexp_t keyparam, const char *name, mpi_ec_t ec)
{
  gcry_err_code_t rc;
  gcry_sexp_t l1;
  gcry_mpi_point_t point;

  l1 = sexp_find_token (keyparam, name, 0);
  if (l1)
    {
      gcry_mpi_t a;

      a = sexp_nth_mpi (l1, 1, GCRYMPI_FMT_OPAQUE);
      sexp_release (l1);
      if (!a)
        return GPG_ERR_INV_OBJ;

      point = mpi_point_new (0);
      rc = _gcry_mpi_ec_decode_point (point, a, ec);
      mpi_free (a);
      if (rc)
        {
          mpi_point_release (point);
          return rc;
        }
    }
  else
    {
      char *tmpname;
      gcry_mpi_t x = NULL;
      gcry_mpi_t y = NULL;
      gcry_mpi_t z = NULL;

      tmpname = xtrymalloc (strlen (name) + 2 + 1);
      if (!tmpname)
        return gpg_err_code_from_syserror ();
      strcpy (stpcpy (tmpname, name), ".x");
      rc = mpi_from_keyparam (&x, keyparam, tmpname, 0);
      if (rc)
        {
          xfree (tmpname);
          return rc;
        }
      strcpy (stpcpy (tmpname, name), ".y");
      rc = mpi_from_keyparam (&y, keyparam, tmpname, 0);
      if (rc)
        {
          mpi_free (x);
          xfree (tmpname);
          return rc;
        }
      strcpy (stpcpy (tmpname, name), ".z");
      rc = mpi_from_keyparam (&z, keyparam, tmpname, 0);
      if (rc)
        {
          mpi_free (y);
          mpi_free (x);
          xfree (tmpname);
          return rc;
        }
      if (!z)
        z = mpi_set_ui (NULL, 1);
      if (x && y)
        point = mpi_point_snatch_set (NULL, x, y, z);
      else
        {
          mpi_free (x);
          mpi_free (y);
          mpi_free (z);
          point = NULL;
        }
      xfree (tmpname);
    }

  if (point)
    *r_a = point;
  return 0;
}



static gpg_err_code_t
mpi_ec_get_elliptic_curve (elliptic_curve_t *E, int *r_flags,
                           gcry_sexp_t keyparam, const char *curvename)
{
  gpg_err_code_t errc;
  unsigned int nbits;
  gcry_sexp_t l1;

  errc = _gcry_pk_util_get_nbits (keyparam, &nbits);
  if (errc)
    return errc;

  E->model = MPI_EC_WEIERSTRASS;
  E->dialect = ECC_DIALECT_STANDARD;
  E->h = 1;

  if (keyparam)
    {
      /* Parse an optional flags list.  */
      l1 = sexp_find_token (keyparam, "flags", 0);
      if (l1)
        {
          int flags = 0;

          errc = _gcry_pk_util_parse_flaglist (l1, &flags, NULL);
          sexp_release (l1);
          l1 = NULL;
          if (errc)
            goto leave;

          *r_flags |= flags;
        }

      /* Parse the deprecated optional transient-key flag.  */
      l1 = sexp_find_token (keyparam, "transient-key", 0);
      if (l1)
        {
          *r_flags |= PUBKEY_FLAG_TRANSIENT_KEY;
          sexp_release (l1);
        }

      /* Check whether a curve name was given.  */
      l1 = sexp_find_token (keyparam, "curve", 5);

      /* If we don't have a curve name or if override parameters have
         explicitly been requested, parse them.  */
      if (!l1 || (*r_flags & PUBKEY_FLAG_PARAM))
        {
          gcry_mpi_point_t G = NULL;
          gcry_mpi_t cofactor = NULL;

          errc = mpi_from_keyparam (&E->p, keyparam, "p", 0);
          if (errc)
            goto leave;
          errc = mpi_from_keyparam (&E->a, keyparam, "a", 0);
          if (errc)
            goto leave;
          errc = mpi_from_keyparam (&E->b, keyparam, "b", 0);
          if (errc)
            goto leave;
          errc = point_from_keyparam (&G, keyparam, "g", NULL);
          if (errc)
            goto leave;
          if (G)
            {
              _gcry_mpi_point_init (&E->G);
              mpi_point_set (&E->G, G->x, G->y, G->z);
              mpi_point_set (G, NULL, NULL, NULL);
              mpi_point_release (G);
            }
          errc = mpi_from_keyparam (&E->n, keyparam, "n", 0);
          if (errc)
            goto leave;
          errc = mpi_from_keyparam (&cofactor, keyparam, "h", 0);
          if (errc)
            goto leave;
          if (cofactor)
            {
              mpi_get_ui (&E->h, cofactor);
              mpi_free (cofactor);
            }
        }
    }
  else
    l1 = NULL; /* No curvename.  */

  /* Check whether a curve parameter is available and use that to fill
     in missing values.  If no curve parameter is available try an
     optional provided curvename.  If only the curvename has been
     given use that one. */
  if (l1 || curvename || nbits)
    {
      char *name;

      if (l1)
        {
          name = sexp_nth_string (l1, 1);
          sexp_release (l1);
          if (!name)
            {
              errc = GPG_ERR_INV_OBJ; /* Name missing or out of core. */
              goto leave;
            }
        }
      else
        name = NULL;

      errc = _gcry_ecc_fill_in_curve (nbits, name? name : curvename, E, NULL);
      xfree (name);
      if (errc)
        goto leave;
    }

 leave:
  return errc;
}

static gpg_err_code_t
mpi_ec_setup_elliptic_curve (mpi_ec_t ec, int flags,
                             elliptic_curve_t *E, gcry_sexp_t keyparam)
{
  gpg_err_code_t errc = 0;

  ec->G = mpi_point_snatch_set (NULL, E->G.x, E->G.y, E->G.z);
  E->G.x = NULL;
  E->G.y = NULL;
  E->G.z = NULL;
  ec->n = E->n;
  E->n = NULL;
  ec->h = E->h;
  ec->name = E->name;

  /* Now that we know the curve name we can look for the public key
     Q.  point_from_keyparam needs to know the curve parameters so
     that it is able to use the correct decompression.  Parsing
     the private key D could have been done earlier but it is less
     surprising if we do it here as well.  */
  if (keyparam)
    {
      int is_opaque_bytes = ((ec->dialect == ECC_DIALECT_ED25519
                              && (flags & PUBKEY_FLAG_EDDSA))
                             || (ec->dialect == ECC_DIALECT_SAFECURVE));

      errc = point_from_keyparam (&ec->Q, keyparam, "q", ec);
      if (errc)
        return errc;
      errc = mpi_from_keyparam (&ec->d, keyparam, "d", is_opaque_bytes);

      /* Size of opaque bytes should match size of P.  */
      if (!errc && ec->d && is_opaque_bytes)
        {
          unsigned int n = mpi_get_nbits (ec->d);
          unsigned int len;

          len = (ec->nbits+7)/8;
          /* EdDSA requires additional bit for sign.  */
          if ((ec->nbits%8) == 0 && ec->model == MPI_EC_EDWARDS)
            len++;

          if ((n+7)/8 != len)
            {
              if (ec->dialect == ECC_DIALECT_ED25519)
                {
                  /*
                   * GnuPG (<= 2.2) or OpenPGP implementations with no
                   * SOS support may remove zeros at the beginning.
                   * Recover those zeros.
                   */
                  /*
                   * Also, GnuPG (<= 2.2) may add additional zero at
                   * the beginning, when private key is moved from
                   * OpenPGP to gpg-agent.  Remove such a zero-prefix.
                   */
                  const unsigned char *buf;
                  unsigned char *value;

                  buf = mpi_get_opaque (ec->d, &n);
                  if (!buf)
                    return GPG_ERR_INV_OBJ;

                  value = xtrymalloc_secure (len);
                  if (!value)
                    return gpg_err_code_from_syserror ();

                  if ((n+7)/8 < len)
                    /* Recover zeros.  */
                    {
                      memset (value, 0, len - (n+7)/8);
                      memcpy (value + len - (n+7)/8, buf, (n+7)/8);
                    }
                  else if ((n+7)/8 == len + 1)
                    /* Remove a zero.  */
                    memcpy (value, buf+1, len);
                  else
                    {
                      xfree (value);
                      return GPG_ERR_INV_OBJ;
                    }

                  mpi_set_opaque (ec->d, value, len*8);
                }
              else
                {
                  if (DBG_CIPHER)
                    log_debug ("scalar size (%d) != prime size (%d)",
                               (n+7)/8, len);

                  errc = GPG_ERR_INV_OBJ;
                }
            }
        }
    }

  return errc;
}

gpg_err_code_t
_gcry_mpi_ec_internal_new (mpi_ec_t *r_ec, int *r_flags, const char *name_op,
                           gcry_sexp_t keyparam, const char *curvename)
{
  gpg_err_code_t errc;
  elliptic_curve_t E;
  mpi_ec_t ec;

  *r_ec = NULL;

  memset (&E, 0, sizeof E);
  errc = mpi_ec_get_elliptic_curve (&E, r_flags, keyparam, curvename);
  if (errc)
    goto leave;

  ec = _gcry_mpi_ec_p_internal_new (E.model, E.dialect, *r_flags,
                                    E.p, E.a, E.b);
  if (!ec)
    goto leave;

  errc = mpi_ec_setup_elliptic_curve (ec, *r_flags, &E, keyparam);
  if (errc)
    {
      _gcry_mpi_ec_free (ec);
      goto leave;
    }
  else
    *r_ec = ec;

  if (!errc && DBG_CIPHER)
    {
      gcry_mpi_t mpi_q = NULL;
      gcry_sexp_t l1;
      char msg[80];

      l1 = sexp_find_token (keyparam, "q", 0);
      if (l1)
        {
          mpi_q = sexp_nth_mpi (l1, 1, GCRYMPI_FMT_OPAQUE);
          sexp_release (l1);
        }

      log_debug ("%s info: %s/%s%s\n", name_op,
                 _gcry_ecc_model2str (ec->model),
                 _gcry_ecc_dialect2str (ec->dialect),
                 (*r_flags & PUBKEY_FLAG_EDDSA)? "+EdDSA" : "");
      if (ec->name)
        log_debug  ("%s name: %s\n", name_op, ec->name);
      snprintf (msg, sizeof msg, "%s    p", name_op);
      log_printmpi (msg, ec->p);
      snprintf (msg, sizeof msg, "%s    a", name_op);
      log_printmpi (msg, ec->a);
      snprintf (msg, sizeof msg, "%s    b", name_op);
      log_printmpi (msg, ec->b);
      snprintf (msg, sizeof msg, "%s  g", name_op);
      log_printpnt (msg, ec->G, NULL);
      snprintf (msg, sizeof msg, "%s    n", name_op);
      log_printmpi (msg, ec->n);
      log_debug ("%s    h:+%02x\n", name_op, ec->h);
      if (mpi_q)
        {
          snprintf (msg, sizeof msg, "%s    q", name_op);
          log_printmpi (msg, mpi_q);
          mpi_free (mpi_q);
        }
      if (!fips_mode () && ec->d)
        {
          snprintf (msg, sizeof msg, "%s    d", name_op);
          log_printmpi (msg, ec->d);
        }
    }

 leave:
  _gcry_ecc_curve_free (&E);
  return errc;
}

/* This function creates a new context for elliptic curve operations.
   Either KEYPARAM or CURVENAME must be given.  If both are given and
   KEYPARAM has no curve parameter, CURVENAME is used to add missing
   parameters.  On success 0 is returned and the new context stored at
   R_CTX.  On error NULL is stored at R_CTX and an error code is
   returned.  The context needs to be released using
   gcry_ctx_release.  */
gpg_err_code_t
_gcry_mpi_ec_new (gcry_ctx_t *r_ctx,
                  gcry_sexp_t keyparam, const char *curvename)
{
  gpg_err_code_t errc;
  elliptic_curve_t E;
  gcry_ctx_t ctx = NULL;
  int flags = 0;
  mpi_ec_t ec;

  *r_ctx = NULL;

  memset (&E, 0, sizeof E);
  errc = mpi_ec_get_elliptic_curve (&E, &flags, keyparam, curvename);
  if (errc)
    goto leave;

  errc = _gcry_mpi_ec_p_new (&ctx, E.model, E.dialect, flags, E.p, E.a, E.b);
  if (errc)
    goto leave;

  ec = _gcry_ctx_get_pointer (ctx, CONTEXT_TYPE_EC);
  errc = mpi_ec_setup_elliptic_curve (ec, flags, &E, keyparam);
  if (errc)
    goto leave;

  *r_ctx = ctx;
  ctx = NULL;

 leave:
  _gcry_ecc_curve_free (&E);
  _gcry_ctx_release (ctx);
  return errc;
}


/* Return the parameters of the curve NAME as an S-expression.  */
gcry_sexp_t
_gcry_ecc_get_param_sexp (const char *name)
{
  unsigned int nbits;
  elliptic_curve_t E;
  mpi_ec_t ctx;
  gcry_mpi_t g_x, g_y;
  gcry_mpi_t pkey[5];
  gcry_sexp_t result;
  int i;

  memset (&E, 0, sizeof E);
  if (_gcry_ecc_fill_in_curve (0, name, &E, &nbits))
    return NULL;

  g_x = mpi_new (0);
  g_y = mpi_new (0);
  ctx = _gcry_mpi_ec_p_internal_new (E.model,
                                     E.dialect,
                                     0,
                                     E.p, E.a, E.b);
  if (_gcry_mpi_ec_get_affine (g_x, g_y, &E.G, ctx))
    log_fatal ("ecc get param: Failed to get affine coordinates\n");
  _gcry_mpi_ec_free (ctx);
  _gcry_mpi_point_free_parts (&E.G);

  pkey[0] = E.p;
  pkey[1] = E.a;
  pkey[2] = E.b;
  pkey[3] = _gcry_ecc_ec2os (g_x, g_y, E.p);
  pkey[4] = E.n;

  mpi_free (g_x);
  mpi_free (g_y);

  if (sexp_build (&result, NULL,
                  "(public-key(ecc(p%m)(a%m)(b%m)(g%m)(n%m)(h%u)))",
                  pkey[0], pkey[1], pkey[2], pkey[3], pkey[4], E.h))
    result = NULL;

  for (i=0; i < DIM (pkey); i++)
    _gcry_mpi_release (pkey[i]);

  return result;
}


/* Return an MPI (or opaque MPI) described by NAME and the context EC.
   If COPY is true a copy is returned, if not a const MPI may be
   returned.  In any case mpi_free must be used.  */
gcry_mpi_t
_gcry_ecc_get_mpi (const char *name, mpi_ec_t ec, int copy)
{
  if (!*name)
    return NULL;

  if (!strcmp (name, "p") && ec->p)
    return mpi_is_const (ec->p) && !copy? ec->p : mpi_copy (ec->p);
  if (!strcmp (name, "a") && ec->a)
    return mpi_is_const (ec->a) && !copy? ec->a : mpi_copy (ec->a);
  if (!strcmp (name, "b") && ec->b)
    return mpi_is_const (ec->b) && !copy? ec->b : mpi_copy (ec->b);
  if (!strcmp (name, "n") && ec->n)
    return mpi_is_const (ec->n) && !copy? ec->n : mpi_copy (ec->n);
  if (!strcmp (name, "h"))
    {
      gcry_mpi_t h = _gcry_mpi_get_const (ec->h);

      return !copy? h : mpi_set (NULL, h);
    }
  if (!strcmp (name, "d") && ec->d)
    return mpi_is_const (ec->d) && !copy? ec->d : mpi_copy (ec->d);

  /* Return a requested point coordinate.  */
  if (!strcmp (name, "g.x") && ec->G && ec->G->x)
    return mpi_is_const (ec->G->x) && !copy? ec->G->x : mpi_copy (ec->G->x);
  if (!strcmp (name, "g.y") && ec->G && ec->G->y)
    return mpi_is_const (ec->G->y) && !copy? ec->G->y : mpi_copy (ec->G->y);
  if (!strcmp (name, "q.x") && ec->Q && ec->Q->x)
    return mpi_is_const (ec->Q->x) && !copy? ec->Q->x : mpi_copy (ec->Q->x);
  if (!strcmp (name, "q.y") && ec->Q && ec->Q->y)
    return mpi_is_const (ec->Q->y) && !copy? ec->Q->y : mpi_copy (ec->Q->y);

  /* If the base point has been requested, return it in standard
     encoding.  */
  if (!strcmp (name, "g") && ec->G)
    return _gcry_mpi_ec_ec2os (ec->G, ec);

  /* If the public key has been requested, return it by default in
     standard uncompressed encoding or if requested in other
     encodings.  */
  if (*name == 'q' && (!name[1] || name[1] == '@'))
    {
      /* If only the private key is given, compute the public key.  */
      if (!ec->Q)
        ec->Q = _gcry_ecc_compute_public (NULL, ec);

      if (!ec->Q)
        return NULL;

      if (name[1] != '@')
        return _gcry_mpi_ec_ec2os (ec->Q, ec);

      if (!strcmp (name+2, "eddsa") && ec->model == MPI_EC_EDWARDS)
        {
          unsigned char *encpk;
          unsigned int encpklen;

          if (!_gcry_ecc_eddsa_encodepoint (ec->Q, ec, NULL, NULL, 0,
                                            &encpk, &encpklen))
            return mpi_set_opaque (NULL, encpk, encpklen*8);
        }
    }

  return NULL;
}


/* Return a point described by NAME and the context EC.  */
gcry_mpi_point_t
_gcry_ecc_get_point (const char *name, mpi_ec_t ec)
{
  if (!strcmp (name, "g") && ec->G)
    return point_copy (ec->G);
  if (!strcmp (name, "q"))
    {
      /* If only the private key is given, compute the public key.  */
      if (!ec->Q)
        ec->Q = _gcry_ecc_compute_public (NULL, ec);

      if (ec->Q)
        return point_copy (ec->Q);
    }

  return NULL;
}


/* Store the MPI NEWVALUE into the context EC under NAME. */
gpg_err_code_t
_gcry_ecc_set_mpi (const char *name, gcry_mpi_t newvalue, mpi_ec_t ec)
{
  gpg_err_code_t rc = 0;

  if (!*name)
    ;
  else if (!strcmp (name, "p"))
    {
      mpi_free (ec->p);
      ec->p = mpi_copy (newvalue);
      _gcry_mpi_ec_get_reset (ec);
    }
  else if (!strcmp (name, "a"))
    {
      mpi_free (ec->a);
      ec->a = mpi_copy (newvalue);
      _gcry_mpi_ec_get_reset (ec);
    }
  else if (!strcmp (name, "b"))
    {
      mpi_free (ec->b);
      ec->b = mpi_copy (newvalue);
    }
  else if (!strcmp (name, "n"))
    {
      mpi_free (ec->n);
      ec->n = mpi_copy (newvalue);
    }
  else if (!strcmp (name, "h"))
    {
      mpi_get_ui (&ec->h, newvalue);
    }
  else if (*name == 'q' && (!name[1] || name[1] == '@'))
    {
      if (newvalue)
        {
          if (!ec->Q)
            ec->Q = mpi_point_new (0);
          rc = _gcry_mpi_ec_decode_point (ec->Q, newvalue, ec);
        }
      if (rc || !newvalue)
        {
          _gcry_mpi_point_release (ec->Q);
          ec->Q = NULL;
        }
      /* Note: We assume that Q matches d and thus do not reset d.  */
    }
  else if (!strcmp (name, "d"))
    {
      mpi_free (ec->d);
      ec->d = mpi_copy (newvalue);
      if (ec->d)
        {
          /* We need to reset the public key because it may not
             anymore match.  */
          _gcry_mpi_point_release (ec->Q);
          ec->Q = NULL;
        }
    }
  else
   rc = GPG_ERR_UNKNOWN_NAME;

  return rc;
}


/* Store the point NEWVALUE into the context EC under NAME.  */
gpg_err_code_t
_gcry_ecc_set_point (const char *name, gcry_mpi_point_t newvalue, mpi_ec_t ec)
{
  if (!strcmp (name, "g"))
    {
      _gcry_mpi_point_release (ec->G);
      ec->G = point_copy (newvalue);
    }
  else if (!strcmp (name, "q"))
    {
      _gcry_mpi_point_release (ec->Q);
      ec->Q = point_copy (newvalue);
    }
  else
    return GPG_ERR_UNKNOWN_NAME;

  return 0;
}
