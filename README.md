# ALS The CP/M Card for MAME

[한국어](README.md) | [English](README.en.md)

이 저장소는 ALS The CP/M Card를 MAME의 Apple II 버스에 추가하는 변경 사항과 CP/M 디스크 이미지 및 참고 문서를 제공합니다. MAME 전체 소스는 포함하지 않습니다.

## 구성

- `src/devices/bus/a2bus/a2cpmcard.cpp`, `a2cpmcard.h`: 6 MHz Z80B, 카드 RAM, Apple II 메모리/DMA 연결, 뱅크 전환 및 슬롯 제어 구현
- `src/lib/formats/fdi_dsk.cpp`: MAME의 PC-98 FDI 포맷 구현을 재사용하는 래퍼
- `patches/mame-cpmcard.patch`: 새 파일과 MAME 빌드 목록/Apple II 슬롯 등록을 적용하는 패치
- `patches/mame-apple2e-backspace.patch`: Apple IIe Backspace 수정 패치
- `CPM/`: CP/M 디스크 이미지와 카드 참고 문서
- `screenshots/cpm-card-boot.png`: CP/M 카드 장착 상태의 현재 실행 화면

## MAME에 적용하고 빌드

이 변경은 MAME 커밋 `84981cc15cfc1429e48f098d80c6dd97bc82a0bc`를 기준으로 작성했습니다. MAME와 이 저장소를 같은 상위 폴더에 내려받고, 해당 MAME 커밋에서 패치를 적용합니다.

```bash
git clone https://github.com/mamedev/mame.git
git clone https://github.com/mildstorm/TheCPMCard.git
cd mame
git checkout 84981cc15cfc1429e48f098d80c6dd97bc82a0bc
git apply ../TheCPMCard/patches/mame-cpmcard.patch
git apply ../TheCPMCard/patches/mame-apple2e-backspace.patch
```

두 번째 패치는 Apple IIe의 Delete 키(호스트 Backspace 키)를 ASCII Backspace(`0x08`)로 매핑해 CP/M 명령행 편집이 동작하도록 합니다.

MAME 루트에서 Apple II 계열을 빌드합니다.

```bash
make -j$(nproc) SUBTARGET=apple2e SOURCES=src/mame/apple/apple2e.cpp REGENIE=1
```

운영체제별 필수 빌드 도구와 설정은 [MAME 컴파일 안내](https://docs.mamedev.org/initialsetup/compilingmame.html)를 참고하세요.

## CP/M 카드 실행

ALS The CP/M Card는 슬롯 4 또는 7에, 5.25인치 Disk II 컨트롤러는 슬롯 6에 장착합니다. 아래 예에서는 기본 슬롯 6 Disk II 컨트롤러를 유지하고 CP/M 카드를 슬롯 4에 둡니다. CP/M Plus 시스템 디스크를 첫 번째 드라이브에, GSX-80 D1을 두 번째 드라이브에 연결합니다.

MAME 루트 디렉터리에서 실행합니다. 이 예는 두 저장소를 같은 상위 폴더에 clone한 경우입니다.

```bash
./apple2e.exe apple2e -window -nomaximize -sl4 cpmcard \
  -flop1 "../TheCPMCard/CPM/CPM.Disk1.DSK" \
  -flop2 "../TheCPMCard/CPM/GSX-80 VERSION 1.1/GSX80D1.DSK"
```

MAME는 기본적으로 전체 화면으로 실행됩니다. `-window -nomaximize`를 사용하면 작은 창으로 열리며, 창 테두리를 드래그해 크기를 조절할 수 있습니다.

시작 메뉴에서 `4`를 눌러 `A>` 프롬프트로 이동합니다. 호스트 Backspace 키를 누르면 앞 글자가 지워집니다.

## 현재 실행 화면

아래는 위 명령으로 실행해 CP/M Plus 3.01C가 표시된 화면입니다.

![ALS The CP/M Card running in MAME](screenshots/cpm-card-boot.png)

## 디스크와 참고 문서

`CPM/` 폴더에는 CP/M 2.2 및 CP/M Plus 디스크, ProDOS 디스크와 카드 관련 참고 PDF가 있습니다. 일부 디스크 이미지와 문서는 제3자 저작물일 수 있으므로 사용 및 배포 시 해당 권리와 라이선스를 확인하세요.
