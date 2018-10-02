# hacPack - Docs

## TitleID: --titleid
Title id is 8-bytes hex value which connects ncas to earch other  
Valid Title id range is: 0100000000000000 - 01ffffffffffffff  
If you are repacking ncas, make sure to use the original title id of nca  
##### Switch Title ID template:
Application: 01xxxxxxxxxxxxxx000  
Patch(Update): Application + 0x800  
AddOn(DLC): Application + 0x1000 + 0x01-0xff

## Section Encryption Type: --plaintext
hacPack use AES-CTR encryption for section by default.  
You can use --plaintext to change section encryption type to unencrypted (plaintext).  
Logo section in program nca is always plaintext.

## Key generation: --keygeneration
Keygeneration is a key that hacPack use to encrypt key area in ncas.  
It is a number between 1-32 and it describes the 'key_area_key_application' key that hacPack use in encryption.  
Firmwares always support applications with keygenerations up to the keygeneration they ship with.  

Keygeneration | Firmware
--------------| --------
1 | 1.0.0 - 2.3.0
2 | 3.0.0
3 | 3.0.1 - 3.0.2
4 | 4.0.0 - 4.1.0
5 | 5.0.0 - 5.1.0
6 | 6.0.0

## Key area key 2: --keyareakey
This is 16-bytes key which hacPack use to encrypt sections.  
There's a default key area key 2 in hacPack and you can change it by --keyareakey option.

## SDK Version: --sdkverison
There's a field in ncas which describes the version of sdk that is used to make nca.  
Overall, it's not an important field but it must be greater than 000B0000 (0.11.0.0).  
Valid SDK Version range is: 000B0000 - 00FFFFFF

## Program NCA: --ncatype program
Program NCA contains 3 sections  
Section 0 (known as exefs) contains main and main.npdm, it also may contain rtl and subsdks  
Section 1 (known as romfs) contains romfs data  
Section 2 (known as logo) contains logo data including "NintendoLogo.png" and "StartupMovie.gif"  
You can use --nologo to skip logo and --noromfs to skip romfs section in program nca  
```
*nix: hacpack -o ./out/ --type nca --ncatype program --titleid 0104444444444000 --exefsdir ./exefs/ --romfsdir ./romfs/ --logodir= ./logo/  
Windows: hacpack.exe -o .\out\ --type nca --ncatype program --titleid 0104444444444000 --exefsdir .\exefs\ --romfsdir .\romfs\ --logodir= .\logo\
```

## Control NCA: --ncatype control
Control NCA contains 1 section  
Section 0 (known as romfs) contains control.nacp and icons with icon_{lang}.dat  
```
*nix: hacpack -o ./out/ --type nca --ncatype control --titleid 0104444444444000 --romfsdir ./control/
Windows: hacpack.exe -o .\out\ --type nca --ncatype control --titleid 0104444444444000 --romfsdir .\control\
```
## Manual NCA: --ncatype manual
Manual NCA contains 1 section  
It contains "Legla Information" or "Offline-Manual" html documents  
```
*nix: hacpack -o ./out/ --type nca --ncatype manual --titleid 0104444444444000 --romfsdir ./manual/
Windows: hacpack.exe -o .\out\ --type nca --ncatype manual --titleid 0104444444444000 --romfsdir .\manual\
```
## Data NCA: --ncatype data
Data NCA contains 1 sections  
```
*nix: hacpack -o ./out/ --type nca --ncatype data --titleid 0104444444444000 --romfsdir ./data/
Windows: hacpack.exe -o .\out\ --type nca --ncatype data --titleid 0104444444444000 --romfsdir .\data\
```
## PublicData NCA: --ncatype publicdata
PublicData NCA contains 1 sections  
```
*nix: hacpack -o ./out/ --type nca --ncatype publicdata --titleid 0104444444444001 --romfsdir ./publicdata/
Windows: hacpack.exe -o .\out\ --type nca --ncatype publicdata --titleid 0104444444444001 --romfsdir .\publicdata\
```

## Metadata NCA: --ncatype meta
Metadata NCA contains 1 section  
There's a file called cnmt in metadata nca which contains information about other ncas  
You must specify --titletype option for creating metadata nca. Title type  "application" is for apps and games, "addon" is for dlcs  
"application contains" program and control ncas, it may also contain manual(legal information, offline-manual html) and data ncas.  
"addon" only contains publicdata nca  
```
*nix: hacpack -o ./out/ --type nca --ncatype meta --titleid 0104444444444000 --programnca ./nca/00000000000000000000000000000001.nca
  --controlnca ./nca/00000000000000000000000000000002.nca --legalnca ./nca/00000000000000000000000000000003.nca
  --htmldocnca ./nca/00000000000000000000000000000004.nca --datanca ./nca/00000000000000000000000000000005.nca  
Windows: hacpack.exe -o .\out\ --type nca --ncatype meta --titleid 0104444444444000 --programnca .\nca\00000000000000000000000000000001.nca
  --controlnca .\nca\00000000000000000000000000000002.nca --legalnca .\nca\00000000000000000000000000000003.nca
  --htmldocnca .\nca\00000000000000000000000000000004.nca --datanca .\nca\00000000000000000000000000000005.nca  
```

## NSP: --type nsp
NSP is a container for ncas  
You must set your ncas folder with --ncadir option
```
*nix: hacpack -o ./nsp/ --type nsp --ncadir ./ncas/ --titleid 0104444444444000
Windows: hacpack.exe -o .\nsp\ --type nsp --ncadir .\ncas\ --titleid 0104444444444000
```