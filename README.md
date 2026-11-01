# ALS The CP/M Card for MAME

이 저장소는 ALS The CP/M Card를 MAME의 Apple II 버스에 추가하는 변경 사항과 부팅용 CP/M 디스크 이미지를 제공합니다. MAME 전체 소스는 포함하지 않습니다.

## 구성

- `src/devices/bus/a2bus/a2cpmcard.cpp`, `a2cpmcard.h`: 6 MHz Z80B, 카드 RAM, Apple II 메모리/DMA 연결, 뱅크 전환 및 슬롯 제어 구현
- `src/lib/formats/fdi_dsk.cpp`: MAME의 PC-98 FDI 포맷 구현을 재사용하는 래퍼
- `patches/mame-cpmcard.patch`: 위 파일을 추가하고 MAME 빌드 목록 및 Apple II 슬롯 카드 목록에 등록하는 패치
- `CPM/`: 부팅 디스크, CP/M 디스크 이미지, 참고 문서

## MAME에 적용하고 MSYS2에서 빌드

이 변경은 MAME 커밋 `84981cc15cfc1429e48f098d80c6dd97bc82a0bc`를 기준으로 작성했습니다. 동일한 기준 소스 트리에서 적용하는 방법은 다음과 같습니다. 최신 MAME에서 패치가 적용되지 않으면 먼저 소스 변경을 확인하고 필요한 부분을 조정하세요.

MSYS2의 **UCRT64** 셸에서 MAME와 이 저장소를 같은 상위 폴더에 내려받습니다.

```bash
git clone https://github.com/mamedev/mame.git
git clone https://github.com/mildstorm/TheCPMCard.git
cd mame
git checkout 84981cc15cfc1429e48f098d80c6dd97bc82a0bc
git apply ../TheCPMCard/patches/mame-cpmcard.patch
```

MAME 루트에서 Apple II 계열만 빌드합니다.

```bash
make -j$(nproc) SUBTARGET=apple2e SOURCES=src/mame/apple/apple2e.cpp REGENIE=1
```

MAME의 Windows 빌드는 MSYS2와 MinGW/UCRT 도구를 사용합니다. 필요한 패키지와 일반 빌드 옵션은 [MAME 컴파일 안내](https://docs.mamedev.org/initialsetup/compilingmame.html)를 참고하세요.

## ALS CP/M Card 실행

빌드한 실행 파일을 MAME 루트에서 실행하고, 슬롯 6에 CP/M 카드를 지정한 뒤 `BOOT.DSK`를 첫 번째 플로피 드라이브에 넣습니다.

```bash
./apple2e.exe apple2e -sl6 cpmcard -flop1 ../TheCPMCard/CPM/BOOT.DSK
```

`-sl6 cpmcard`는 Apple IIe의 슬롯 6에 ALS 카드를 장착합니다. 카드가 `Cn00` 슬롯 주소 쓰기를 감지하면 Apple II CPU를 DMA로 멈추고 Z80을 실행합니다. `-flop1`은 `CPM/BOOT.DSK`를 첫 번째 디스크 드라이브에 연결합니다. 다른 CP/M 디스크로 시작하려면 경로를 해당 `.DSK` 파일로 바꾸세요.

## CPM 디스크와 문서

`CPM/` 폴더에는 `BOOT.DSK`, CP/M 및 CP/M Plus 디스크 이미지, ProDOS 디스크 이미지와 카드 참고 PDF가 있습니다. 일부 디스크 이미지와 문서는 제3자 저작물일 수 있으므로 사용 및 배포 시 해당 권리와 라이선스를 확인하세요.
