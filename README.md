# ALS The CP/M Card for MAME

[한국어](README.md) | [English](README.en.md)

이 저장소는 ALS The CP/M Card를 MAME의 Apple II 버스에 추가하는 변경 사항과 CP/M 디스크 이미지 및 참고 문서를 제공합니다. MAME 전체 소스는 포함하지 않습니다.

## 구성

- `src/devices/bus/a2bus/a2cpmcard.cpp`, `a2cpmcard.h`: 6 MHz Z80B, 카드 RAM, Apple II 메모리/DMA 연결, 뱅크 전환 및 슬롯 제어 구현
- `src/lib/formats/fdi_dsk.cpp`: MAME의 PC-98 FDI 포맷 구현을 재사용하는 래퍼
- `patches/mame-cpmcard.patch`: 새 파일과 MAME 빌드 목록/Apple II 슬롯 등록, a2bus의 `/IRQ` 변화 통지(`a2bus.cpp/.h`, `bus_irq_w()`)를 적용하는 패치
- `patches/mame-apple2e-backspace.patch`: Apple IIe Backspace 수정 패치
- `CPM/`: CP/M 디스크 이미지와 카드 참고 문서
- `screenshots/cpm-card-boot.png`: CP/M 카드 장착 상태의 현재 실행 화면
- `photos/`: 실제 카드 사진(부품면·납땜면 스캔과 부분 확대 사진)

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

## 하드웨어 모델

실제 보드 사진(PCB "DIGITAL RESEARCH / Advanced Logic Systems", 08-83)과 `CPM/CPM Card Schematic.pdf`(rev A, 05-04-83)를 기준으로 구현했습니다.

| 부품 | 기능 |
|------|------|
| U15 Z0840006PSC | Z80B, 12 MHz 크리스털(Y1) / 2 = 6 MHz |
| U2-U5, U7-U10 M5K4164S-15 | 64K×1 DRAM 8개 = 카드 RAM 64K |
| U26 Raytheon R29623 (20핀 바이폴라 PROM) | Z80 → Apple 주소 변환, 256바이트 페이지 단위 |
| U22 74S113 | 6502/Z80 버스 소유권 토글 + Z80 클럭 분주 |
| U6 74LS74 | M1 클럭 연장 (DRAM 타이밍) |
| U27 74LS74 | 뱅크 래치 + 62 Hz 틱 플립플롭 |
| U1 4020 | Apple Φ1 / 2^14 ≈ 62 Hz 실시간 클럭 틱 |

부품면: [photos/Scan_20261112_233251.jpg](photos/Scan_20261112_233251.jpg), 납땜면: [photos/Scan_20261112_233343.jpg](photos/Scan_20261112_233343.jpg)

카드에는 **펌웨어 ROM이 없습니다**. 모든 코드는 6502가 디스크에서 읽어 옵니다. 프로그램된 부품은 U26 PROM 하나이며 아직 덤프되지 않았습니다.

- **제어권 전환**: `$Cnxx` 쓰기(6502가 직접, 또는 Z80이 DMA로)가 있을 때마다 버스 소유권이 바뀌고, Z80이 버스를 가진 동안 카드가 /DMA를 겁니다. Z80 전체가 멈추지는 않습니다. 6502가 버스를 가진 동안 Apple 메모리에 접근할 때만 /WAIT이 걸리므로, 카드 DRAM에서는 6502와 동시에 실행될 수 있습니다(MAME의 재시도 가능한 메모리 접근으로 구현). `$Cnxx` 읽기는 해독하지 않습니다(오픈 버스).
- **뱅크 래치(U27)**: Z80의 모든 I/O 쓰기에서 D0를 래치합니다. 포트 주소는 해독하지 않습니다. CP/M 3 BIOS는 포트 00에 `F0`(뱅크 0) / `FF`(뱅크 1)를 씁니다. Z80 I/O 읽기는 D0 = Apple /IRQ, D1-D7 = 1입니다.
- **메모리 맵**: 뱅크 0(리셋 상태)에서 Z80 `$0000-$EFFF`는 U26을 거쳐 Apple 메모리로 갑니다. 테이블은 Z80 + `$9000`에 `$Cxxx`/`$Fxxx` 페이지를 맞바꾼 형태로 합성했습니다(Z80 `$0000-$2FFF` → `$9000-$BFFF`, `$3000` → `$F000`, `$4000-$5FFF` → `$D000-$EFFF`, `$6000` → `$C000` I/O, `$7000-$EFFF` → `$0000-$7FFF`). PROM에는 Z80 A15가 반전되어 들어갑니다. 뱅크 1에서는 `$0000-$EFFF`가 카드 DRAM입니다. `$F000-$FFFF`는 두 뱅크 모두 카드 DRAM입니다.
- **실시간 클럭**: U1이 Apple Φ1을 2^14로 나눕니다(≈62.3 Hz). Z80 /INT = 틱 대기 OR Apple /IRQ(슬롯 30번 핀)입니다. 인터럽트 응답 때 틱을 지우고 `$F7`(IM 0에서 `RST 30h`)을 버스에 올립니다. 3.01C BIOS의 시계 핸들러(`$FB94`)는 60틱을 1초로 세므로 `DATE`의 시계가 흐르며, 실제 카드처럼 약 4% 빠릅니다.
- **Z80 클럭**: 뱅크 1에서는 U6이 명령어 읽기마다 12 MHz 펄스 하나를 빼서 M1이 4.5 T-state가 됩니다(두 번 읽을 때마다 1 T-state 추가로 구현). 뱅크 0에서는 Apple Φ0가 high인 동안에만 분주기가 돌고, Φ0는 매 Apple 사이클의 정확히 50%가 high이므로 평균 3 MHz입니다(클럭 배율 0.5, 두 크리스털은 위상이 고정되지 않음).

## 디스크와 참고 문서

`CPM/` 폴더에는 CP/M 2.2 및 CP/M Plus 디스크, ProDOS 디스크와 카드 관련 참고 PDF가 있습니다. 일부 디스크 이미지와 문서는 제3자 저작물일 수 있으므로 사용 및 배포 시 해당 권리와 라이선스를 확인하세요.
