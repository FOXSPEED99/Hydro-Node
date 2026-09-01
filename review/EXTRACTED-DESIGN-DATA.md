# Hydro Node — extracted design data

Machine-extracted from the Altium binary files, supporting `HYDRO-NODE-PRODUCTION-REVIEW-v2.md`.
Regenerate with the OLE/record parsing described below; all coordinates are Altium internal
units converted to mil (1 mil = 0.0254 mm).

## 1. Schematic component list (37 parts)

| Desig | Library reference | Description |
|---|---|---|
| BATT1 | `B2B-XH-A_(LF)(SN)` | Connector Header Through Hole 2 position 0.098 (2.50mm) |
| C1 | `SR215E104MARTR1.` | SR Series 5.08x5.08 mm 100 nF 50 V ±20% Tol. Z5U Radial Lead/SkyCap® |
| C2 | `SR215E104MARTR1.` | SR Series 5.08x5.08 mm 100 nF 50 V ±20% Tol. Z5U Radial Lead/SkyCap® |
| C3 | `SR215E104MARTR1.` | SR Series 5.08x5.08 mm 100 nF 50 V ±20% Tol. Z5U Radial Lead/SkyCap® |
| C4 | `SR215E104MARTR1.` | SR Series 5.08x5.08 mm 100 nF 50 V ±20% Tol. Z5U Radial Lead/SkyCap® |
| C5 | `SR215E104MARTR1.` | SR Series 5.08x5.08 mm 100 nF 50 V ±20% Tol. Z5U Radial Lead/SkyCap® |
| C6 | `SR215E104MARTR1.` | SR Series 5.08x5.08 mm 100 nF 50 V ±20% Tol. Z5U Radial Lead/SkyCap® |
| C7 | `Capacitor_Polarized` | WCAP-ATLL Aluminum Electrolytic Capacitors |
| C8 | `Capacitor_Polarized` | Alum. Electrolytic Cap 10UF 50V 5MM Radial Wcap-atll, Lifetime 4000H +105OC |
| C9 | `Capacitor_Polarized` | Alum. Electrolytic Cap 10UF 50V 5MM Radial Wcap-atll, Lifetime 4000H +105OC |
| C10 | `SR215E104MARTR1.` | SR Series 5.08x5.08 mm 100 nF 50 V ±20% Tol. Z5U Radial Lead/SkyCap® |
| C11 | `SR215E104MARTR1.` | SR Series 5.08x5.08 mm 100 nF 50 V ±20% Tol. Z5U Radial Lead/SkyCap® |
| C12 | `SR215E104MARTR1.` | SR Series 5.08x5.08 mm 100 nF 50 V ±20% Tol. Z5U Radial Lead/SkyCap® |
| D1 | `1d6ec82d83fb6aee721767c19dac4b9` | DIODE SCHOTTKY 40V 1A DO41 |
| J1 | `3bc925449376181038e59eaffc5286c` | CONN HEADER VERT 2POS 2.5MM |
| J2 | `0d3f023dd272f625e17a2be33dea85f` | CONN HEADER VERT 3POS 2.5MM |
| J3 | `299a8cea1bdcbc3624fe62c390fa72c` | CONN HEADER VERT 4POS 2.5MM |
| LS1 | `CPT-1255C-090` | 12 mm, 20 Vp-p, 80 dB, Through Hole, Piezo Audio Transducer Buzzer |
| Q1 | `IRLZ44N` | N-Channel Logic-Level HEXFET Power MOSFET, 55V, 47A, Rds(on) 22mOhm @ Vgs=5V, TO-220AB |
| R1 | `1029c3af85768c4190bb7ad29bc67b5` | RES 100 OHM 1/4W 1% AXIAL |
| R2 | `1029c3af85768c4190bb7ad29bc67b5` | RES 100 OHM 1/4W 1% AXIAL |
| R3 | `1029c3af85768c4190bb7ad29bc67b5` | RES 330 OHM 1/4W 1% AXIAL |
| R4 | `1029c3af85768c4190bb7ad29bc67b5` | RES 100 OHM 1/4W 1% AXIAL |
| R5 | `1029c3af85768c4190bb7ad29bc67b5` | RES 100 OHM 1/4W 1% AXIAL |
| R6 | `1029c3af85768c4190bb7ad29bc67b5` | RES 1MO 1/4W 1% AXIAL |
| R7 | `1029c3af85768c4190bb7ad29bc67b5` | RES 4.7k 1/4W 1% AXIAL |
| R8 | `1029c3af85768c4190bb7ad29bc67b5` | RES 1M OHM 1/4W 1% AXIAL |
| R9 | `1029c3af85768c4190bb7ad29bc67b5` | RES 100 OHM 1/4W 1% AXIAL |
| R10 | `1029c3af85768c4190bb7ad29bc67b5` | RES 1K OHM 1/4W 1% AXIAL |
| R11 | `1029c3af85768c4190bb7ad29bc67b5` | RES 100K OHM 1/4W 1% AXIAL |
| R12 | `1029c3af85768c4190bb7ad29bc67b5` | RES 1M OHM 1/4W 1% AXIAL |
| R13 | `1029c3af85768c4190bb7ad29bc67b5` | RES 100 OHM 1/4W 1% AXIAL |
| R14 | `1029c3af85768c4190bb7ad29bc67b5` | RES 2.2M OHM 1/4W 1% AXIAL |
| S1 | `REED-SWITCH-NO-D4L29` | Magnetic reed switch, normally open, glass body 4.0 x 29.0 mm, axial leads bent for 35.0 mm through-hole pitch |
| U1 | `ARDUINO_PRO_MINI` | This board was developed for applications and installations where space is premium and projects are made as pe |
| U2 | `74HC74N,652` | Flip Flop 2 Element D-Type 1 Bit Positive Edge 14-DIP (0.300, 7.62mm) |
| U3 | `Ra-02` | Ai-Thinker Ra-02 LoRa transceiver module, SX1278, 410-525 MHz, +18 dBm, SPI, IPEX antenna, SMD-16 |

## 2. Extracted schematic netlist

Recovered geometrically: pin connection endpoints, wire segments, point-on-segment tests,
junctions and net labels. Nets with a single member are unconnected pins.

| Net | Pins | Members |
|---|---:|---|
| `GND` | 20 | `C1.2(2)`, `C2.2(2)`, `C3.1(1)`, `C5.1(1)`, `C6.2(2)`, `C7.2(N)`, `C8.2(N)`, `J1.1(1)`, `J2.2(2)`, `J3.1(1)`, `LS1.N(N)`, `Q1.2(D)`, `U1.JP1_5(GND)`, `U1.JP1_6(GND)`, `U1.JP6_2(GND_1)`, `U1.JP7_9(GND_2)`, `U3.1(GND)`, `U3.16(GND)`, `U3.2(GND)`, `U3.9(GND)` |
| `BATT+` | 14 | `BATT1.2(2)`, `C1.1(1)`, `C3.2(2)`, `C4.2(2)`, `C5.2(2)`, `C6.1(1)`, `C7.1(P)`, `C8.1(P)`, `D1.2(A)`, `J3.2(2)`, `R6.1(1)`, `U1.JP1_4(VCC)`, `U1.JP6_4(VCC_1)`, `U3.3(3.3V)` |
| `BATT-` | 12 | `BATT1.1(1)`, `C10.2(2)`, `C11.2(2)`, `C12.2(2)`, `C4.1(1)`, `C9.2(N)`, `Q1.3(S)`, `R14.2(2)`, `R8.1(1)`, `U2.11(2CP)`, `U2.12(2D)`, `U2.7(GND)` |
| `(unnamed)` | 9 | `C10.1(1)`, `C9.1(P)`, `D1.1(C)`, `R12.1(1)`, `S1.2(2)`, `U2.10(2~SD)`, `U2.13(2~RD)`, `U2.14(VCC)`, `U2.4(1~SD)` |
| `(unnamed)` | 5 | `C2.1(1)`, `J1.2(2)`, `R3.1(1)`, `R5.2(2)`, `R6.2(2)` |
| `(unnamed)` | 4 | `C11.1(1)`, `R11.2(2)`, `R12.2(2)`, `U2.1(1~RD)` |
| `(unnamed)` | 4 | `C12.1(1)`, `R13.2(2)`, `R14.1(1)`, `U2.3(1CP)` |
| `(unnamed)` | 3 | `J2.3(3)`, `R7.1(1)`, `U1.JP7_7(D3)` |
| `(unnamed)` | 3 | `J2.1(1)`, `R4.2(2)`, `R7.2(2)` |
| `(unnamed)` | 3 | `Q1.1(G)`, `R10.2(2)`, `R8.2(2)` |
| `(unnamed)` | 2 | `U2.2(1D)`, `U2.6(1~Q)` |
| `(unnamed)` | 2 | `R10.1(1)`, `U2.5(1Q)` |
| `(unnamed)` | 2 | `R11.1(1)`, `U1.JP6_7(A1)` |
| `(unnamed)` | 2 | `R3.2(2)`, `U1.JP6_6(A2)` |
| `(unnamed)` | 2 | `U1.JP6_9(SCK)`, `U3.12(SCK)` |
| `(unnamed)` | 2 | `U1.JP6_10(MISO)`, `U3.13(MISO)` |
| `(unnamed)` | 2 | `U1.JP6_11(MOSI)`, `U3.14(MOSI)` |
| `(unnamed)` | 2 | `U1.JP6_12(D10)`, `U3.15(NSS)` |
| `(unnamed)` | 2 | `U1.JP7_8(D2)`, `U3.5(DIO0)` |
| `(unnamed)` | 2 | `R4.1(1)`, `U1.JP7_6(D4)` |
| `(unnamed)` | 2 | `R5.1(1)`, `U1.JP7_5(D5)` |
| `(unnamed)` | 2 | `R2.2(2)`, `U1.JP7_4(D6)` |
| `(unnamed)` | 2 | `R9.2(2)`, `U1.JP7_3(D7)` |
| `(unnamed)` | 2 | `R1.2(2)`, `U1.JP7_2(D8)` |
| `(unnamed)` | 2 | `U1.JP7_1(D9)`, `U3.4(RESET)` |
| `(unnamed)` | 2 | `J3.3(3)`, `R1.1(1)` |
| `(unnamed)` | 2 | `J3.4(4)`, `R2.1(1)` |
| `(unnamed)` | 2 | `R13.1(1)`, `S1.1(1)` |
| `(unnamed)` | 2 | `LS1.P(P)`, `R9.1(1)` |
| `(unnamed)` | 1 | `U2.9(2Q)` |
| `(unnamed)` | 1 | `U2.8(2~Q)` |
| `(unnamed)` | 1 | `U1.JP3_1(A6)` |
| `(unnamed)` | 1 | `U1.JP3_2(A7)` |
| `(unnamed)` | 1 | `U1.JP2_1(A4)` |
| `(unnamed)` | 1 | `U1.JP2_2(A5)` |
| `(unnamed)` | 1 | `U1.JP6_3(RST_1)` |
| `(unnamed)` | 1 | `U1.JP6_8(A0)` |
| `(unnamed)` | 1 | `U1.JP6_5(A3)` |
| `(unnamed)` | 1 | `U1.JP6_1(RAW)` |
| `(unnamed)` | 1 | `U1.JP1_1(DTR)` |
| `(unnamed)` | 1 | `U1.JP1_2(TXO)` |
| `(unnamed)` | 1 | `U1.JP1_3(RXI)` |
| `(unnamed)` | 1 | `U1.JP7_10(RST_2)` |
| `(unnamed)` | 1 | `U1.JP7_12(TXO_2)` |
| `(unnamed)` | 1 | `U1.JP7_11(RXI_2)` |
| `(unnamed)` | 1 | `U3.6(DIO1)` |
| `(unnamed)` | 1 | `U3.7(DIO2)` |
| `(unnamed)` | 1 | `U3.8(DIO3)` |
| `(unnamed)` | 1 | `U3.10(DIO4)` |
| `(unnamed)` | 1 | `U3.11(DIO5)` |

## 3. PCB board parameters

| Parameter | Value |
|---|---|
| Board outline | (1003.937, 1003.937) → (4153.543, 3366.142) mil |
| Board size | **80.0 × 60.0 mm** |
| Layers | 2 signal (Top, Bottom) |
| Dielectric 1 height | **12.6 mil** (Altium default) → 0.39 mm finished board |
| Copper thickness | 1.4 mil (1 oz) both layers |
| Copper pours | 2 (Top + Bottom), both on net `GND` |
| Vias | 33, all on `GND`, 50 mil pad / 28 mil hole |
| Pads | 140 |
| Source file path in Board6 | `C:\Users\abdul\Desktop\Hydro Node Device\Hydro Node PCB\PCB_Project\Hydro_Node_PCB.$$$` |

### Design rules (all Altium defaults; no net-class rules defined)

| Rule | Values |
|---|---|
| HoleToHoleClearance | `GAP=10mil` |
| DiffPairsRouting | `MAXLIMIT=10mil`, `MINLIMIT=10mil` |
| HoleSize | `MAXLIMIT=137.7953mil`, `MINLIMIT=1mil` |
| ComponentClearance | `GAP=10mil` |
| SolderMaskExpansion | `EXPANSION=4mil` |
| PasteMaskExpansion | `EXPANSION=0mil` |
| RoutingVias | `WIDTH=50mil`, `MINHOLEWIDTH=28mil`, `MINWIDTH=50mil`, `MAXHOLEWIDTH=28mil`, `MAXWIDTH=59.0551mil` |
| Width | `MAXLIMIT=39.3701mil`, `MINLIMIT=11.811mil`, `PREFEREDWIDTH=19.685mil` |
| PolygonConnect | `RELIEFCONDUCTORWIDTH=10mil`, `RELIEFENTRIES=4` |
| PlaneClearance | `CLEARANCE=20mil` |
| PlaneConnect | `RELIEFENTRIES=4`, `RELIEFCONDUCTORWIDTH=10mil` |
| Clearance | `GAP=11.811mil` |

## 4. PCB component placement

| Desig | Side | Origin X (mil) | Origin Y (mil) | Rot | Footprint |
|---|---|---:|---:|---:|---|
| C7 | BOTTOM | -46535.4331 | -48799.2125 | 270 | `WCAP-ATLL_D5H11` |
| U2 | TOP | 3746.4567 | 2726.3779 | 90 | `DIP254P762X420-14` |
| R10 | TOP | 3917.3228 | 2608.2676 | 90 | `FP-MFR-25-MFG` |
| R8 | TOP | 4064.9605 | 2608.2676 | 90 | `FP-MFR-25-MFG` |
| R9 | TOP | 1791.3385 | 2795.2756 | 270 | `FP-MFR-25-MFG` |
| R14 | TOP | 2972.5723 | 2608.2676 | 90 | `FP-MFR-25-MFG` |
| R13 | TOP | 2086.6142 | 1328.7401 | 0 | `FP-MFR-25-MFG` |
| R12 | TOP | 2824.8031 | 2608.2677 | 90 | `FP-MFR-25-MFG` |
| R11 | TOP | 2677.1653 | 2608.2677 | 90 | `FP-MFR-25-MFG` |
| R7 | TOP | 1644.3139 | 2795.2756 | 90 | `FP-MFR-25-MFG` |
| R6 | TOP | 1507.1318 | 2082.6773 | 90 | `FP-MFR-25-MFG` |
| R5 | TOP | 1683.0709 | 2082.6773 | 90 | `FP-MFR-25-MFG` |
| R4 | TOP | 1497.2893 | 2795.2758 | 90 | `FP-MFR-25-MFG` |
| R3 | TOP | 1505.9055 | 2480.315 | 0 | `FP-MFR-25-MFG` |
| R2 | TOP | 1507.1318 | 1564.9607 | 270 | `FP-MFR-25-MFG` |
| R1 | TOP | 1683.0709 | 1564.9607 | 270 | `FP-MFR-25-MFG` |
| LS1 | BOTTOM | 2440.9449 | 3070.8661 | 0 | `CUI_CPT-1255C-090` |
| D1 | TOP | 3868.1102 | 2263.7795 | 0 | `FP-DO-41_Plastic-MFG` |
| C12 | TOP | 3139.7638 | 2263.7795 | 180 | `CAPRR508W50L508T317H508` |
| C11 | TOP | 2844.4882 | 2263.7795 | 180 | `CAPRR508W50L508T317H508` |
| C10 | TOP | 3435.0393 | 2263.7795 | 180 | `CAPRR508W50L508T317H508` |
| C9 | BOTTOM | -47095.4317 | -46929.1338 | 270 | `WCAP-ATLL_D5H11` |
| C8 | BOTTOM | -46279.5276 | -48799.2125 | 270 | `WCAP-ATLL_D5H11` |
| C5 | TOP | 2074.0322 | 2844.4148 | 0 | `CAPRR508W50L508T317H508` |
| C4 | BOTTOM | 3996.063 | 2096.7225 | 0 | `CAPRR508W50L508T317H508` |
| C3 | TOP | 4025.5905 | 3041.3386 | 270 | `CAPRR508W50L508T317H508` |
| C1 | TOP | 1269.685 | 1284.7734 | 180 | `CAPRR508W50L508T317H508` |
| C2 | TOP | 1277.9527 | 1988.1891 | 180 | `CAPRR508W50L508T317H508` |
| BATT1 | BOTTOM | 3987.2047 | 1830.7087 | 90 | `JST_B2B-XH-A_(LF)(SN)` |
| S1 | TOP | 2578.7401 | 1151.5748 | 0 | `REEDSW-THT-D4L29-P35` |
| U1 | TOP | 2224.4094 | 2086.2205 | 0 | `MODULE_ARDUINO_PRO_MINI` |
| U3 | TOP | 3248.0315 | 1736.2205 | 90 | `RA-02_BREAKOUT_THT_2X8` |
| Q1 | TOP | 3120.0788 | 3090.5512 | 270 | `TO220AB_HORZ_TABDOWN_M3` |
| J2 | TOP | 1270.6692 | 2775.5905 | 90 | `FP-B3B-XH-A_LF_SN-MFG` |
| J1 | TOP | 1271.2895 | 2234.252 | 270 | `FP-B2B-XH-AM_LF_SN-MFG` |
| J3 | TOP | 1270.6692 | 1643.7007 | 270 | `FP-B4B-XH-A_LF_SN-MFG` |
| C6 | TOP | 3966.5354 | 1486.2205 | 270 | `CAPRR508W50L508T317H508` |

> C7, C8 and C9 share footprint `WCAP-ATLL_D5H11` and all three have origins ~1.27 m
> off-board. Their **pads** are correctly placed on the board (see §5).

## 5. Pad table (140 pads)

| Desig | X (mil) | Y (mil) | Pad (mm) | Hole (mm) | Layer | Net |
|---|---:|---:|---|---:|---|---|
| (free pad) | 1122.0 | 1122.0 | 2.70 × 2.70 | 2.70 | Multi | `—` |
| (free pad) | 1122.0 | 3248.0 | 2.70 × 2.70 | 2.70 | Multi | `—` |
| (free pad) | 1496.1 | 1122.0 | 2.00 × 2.00 | 0.80 | Multi | `BATT+` |
| (free pad) | 2066.9 | 3159.4 | 2.00 × 2.00 | 0.80 | Multi | `BATT+` |
| (free pad) | 4035.4 | 1122.0 | 2.70 × 2.70 | 2.70 | Multi | `—` |
| (free pad) | 4035.4 | 3248.0 | 2.70 × 2.70 | 2.70 | Multi | `—` |
| BATT1 | 3966.5 | 1781.5 | 1.58 × 1.58 | 1.05 | Multi | `BATT+` |
| BATT1 | 3966.5 | 1879.9 | 1.58 × 1.58 | 1.05 | Multi | `BATT-` |
| C1 | 1169.7 | 1284.8 | 1.22 × 1.22 | 0.71 | Multi | `GND` |
| C1 | 1369.7 | 1284.8 | 1.22 × 1.22 | 0.71 | Multi | `BATT+` |
| C2 | 1178.0 | 1988.2 | 1.22 × 1.22 | 0.71 | Multi | `GND` |
| C2 | 1378.0 | 1988.2 | 1.22 × 1.22 | 0.71 | Multi | `NetC2_1` |
| C3 | 4025.6 | 2941.3 | 1.22 × 1.22 | 0.71 | Multi | `BATT+` |
| C3 | 4025.6 | 3141.3 | 1.22 × 1.22 | 0.71 | Multi | `GND` |
| C4 | 3896.1 | 2096.7 | 1.22 × 1.22 | 0.71 | Multi | `BATT-` |
| C4 | 4096.1 | 2096.7 | 1.22 × 1.22 | 0.71 | Multi | `BATT+` |
| C5 | 1974.0 | 2844.4 | 1.22 × 1.22 | 0.71 | Multi | `GND` |
| C5 | 2174.0 | 2844.4 | 1.22 × 1.22 | 0.71 | Multi | `BATT+` |
| C6 | 3966.5 | 1386.2 | 1.22 × 1.22 | 0.71 | Multi | `GND` |
| C6 | 3966.5 | 1586.2 | 1.22 × 1.22 | 0.71 | Multi | `BATT+` |
| C7 | 3464.6 | 1161.4 | 1.30 × 1.30 | 0.80 | Multi | `BATT+` |
| C7 | 3464.6 | 1240.2 | 1.30 × 1.30 | 0.80 | Multi | `GND` |
| C8 | 3720.5 | 1161.4 | 1.30 × 1.30 | 0.80 | Multi | `BATT+` |
| C8 | 3720.5 | 1240.2 | 1.30 × 1.30 | 0.80 | Multi | `GND` |
| C9 | 2904.6 | 3031.5 | 1.30 × 1.30 | 0.80 | Multi | `NetC9_1` |
| C9 | 2904.6 | 3110.2 | 1.30 × 1.30 | 0.80 | Multi | `BATT-` |
| C10 | 3335.0 | 2263.8 | 1.22 × 1.22 | 0.71 | Multi | `BATT-` |
| C10 | 3535.0 | 2263.8 | 1.22 × 1.22 | 0.71 | Multi | `NetC9_1` |
| C11 | 2744.5 | 2263.8 | 1.22 × 1.22 | 0.71 | Multi | `BATT-` |
| C11 | 2944.5 | 2263.8 | 1.22 × 1.22 | 0.71 | Multi | `NetC11_1` |
| C12 | 3039.8 | 2263.8 | 1.22 × 1.22 | 0.71 | Multi | `BATT-` |
| C12 | 3239.8 | 2263.8 | 1.22 × 1.22 | 0.71 | Multi | `NetC12_1` |
| D1 | 3653.3 | 2263.8 | 1.60 × 1.60 | 1.05 | Multi | `NetC9_1` |
| D1 | 4082.9 | 2263.8 | 1.60 × 1.60 | 1.05 | Multi | `BATT+` |
| J1 | 1250.6 | 2185.0 | 1.55 × 1.55 | 1.00 | Multi | `GND` |
| J1 | 1250.6 | 2283.5 | 1.55 × 1.55 | 1.00 | Multi | `NetC2_1` |
| J2 | 1250.0 | 2677.2 | 1.40 × 1.40 | 0.90 | Multi | `NetJ2_1` |
| J2 | 1250.0 | 2775.6 | 1.40 × 1.40 | 0.90 | Multi | `GND` |
| J2 | 1250.0 | 2874.0 | 1.40 × 1.40 | 0.90 | Multi | `NetJ2_3` |
| J3 | 1250.0 | 1496.1 | 1.40 × 1.40 | 0.90 | Multi | `GND` |
| J3 | 1250.0 | 1594.5 | 1.40 × 1.40 | 0.90 | Multi | `BATT+` |
| J3 | 1250.0 | 1692.9 | 1.40 × 1.40 | 0.90 | Multi | `NetJ3_3` |
| J3 | 1250.0 | 1791.3 | 1.40 × 1.40 | 0.90 | Multi | `NetJ3_4` |
| LS1 | 2342.5 | 3070.9 | 1.41 × 1.41 | 0.90 | Multi | `NetLS1_P` |
| LS1 | 2539.4 | 3070.9 | 1.41 × 1.41 | 0.90 | Multi | `GND` |
| Q1 | 3120.1 | 2990.6 | 1.70 × 2.60 | 1.20 | Multi | `BATT-` |
| Q1 | 3120.1 | 3090.6 | 1.70 × 2.60 | 1.20 | Multi | `GND` |
| Q1 | 3120.1 | 3190.6 | 1.70 × 2.60 | 1.20 | Multi | `NetQ1_1` |
| Q1 | 3486.2 | 3090.6 | 6.80 × 9.30 | 0.00 | Top | `GND` |
| Q1 | 3779.5 | 3090.6 | 9.60 × 5.60 | 3.20 | Multi | `GND` |
| R1 | 1683.1 | 1368.1 | 1.25 × 1.25 | 0.75 | Multi | `NetR1_2` |
| R1 | 1683.1 | 1761.8 | 1.25 × 1.25 | 0.75 | Multi | `NetJ3_3` |
| R2 | 1507.1 | 1368.1 | 1.25 × 1.25 | 0.75 | Multi | `NetR2_2` |
| R2 | 1507.1 | 1761.8 | 1.25 × 1.25 | 0.75 | Multi | `NetJ3_4` |
| R3 | 1309.1 | 2480.3 | 1.25 × 1.25 | 0.75 | Multi | `NetC2_1` |
| R3 | 1702.8 | 2480.3 | 1.25 × 1.25 | 0.75 | Multi | `NetR3_2` |
| R4 | 1497.3 | 2598.4 | 1.25 × 1.25 | 0.75 | Multi | `NetR4_1` |
| R4 | 1497.3 | 2992.1 | 1.25 × 1.25 | 0.75 | Multi | `NetJ2_1` |
| R5 | 1683.1 | 1885.8 | 1.25 × 1.25 | 0.75 | Multi | `NetR5_1` |
| R5 | 1683.1 | 2279.5 | 1.25 × 1.25 | 0.75 | Multi | `NetC2_1` |
| R6 | 1507.1 | 1885.8 | 1.25 × 1.25 | 0.75 | Multi | `BATT+` |
| R6 | 1507.1 | 2279.5 | 1.25 × 1.25 | 0.75 | Multi | `NetC2_1` |
| R7 | 1644.3 | 2598.4 | 1.25 × 1.25 | 0.75 | Multi | `NetJ2_3` |
| R7 | 1644.3 | 2992.1 | 1.25 × 1.25 | 0.75 | Multi | `NetJ2_1` |
| R8 | 4065.0 | 2411.4 | 1.25 × 1.25 | 0.75 | Multi | `BATT-` |
| R8 | 4065.0 | 2805.1 | 1.25 × 1.25 | 0.75 | Multi | `NetQ1_1` |
| R9 | 1791.3 | 2598.4 | 1.25 × 1.25 | 0.75 | Multi | `NetR9_2` |
| R9 | 1791.3 | 2992.1 | 1.25 × 1.25 | 0.75 | Multi | `NetLS1_P` |
| R10 | 3917.3 | 2411.4 | 1.25 × 1.25 | 0.75 | Multi | `NetR10_1` |
| R10 | 3917.3 | 2805.1 | 1.25 × 1.25 | 0.75 | Multi | `NetQ1_1` |
| R11 | 2677.2 | 2411.4 | 1.25 × 1.25 | 0.75 | Multi | `NetR11_1` |
| R11 | 2677.2 | 2805.1 | 1.25 × 1.25 | 0.75 | Multi | `NetC11_1` |
| R12 | 2824.8 | 2411.4 | 1.25 × 1.25 | 0.75 | Multi | `NetC9_1` |
| R12 | 2824.8 | 2805.1 | 1.25 × 1.25 | 0.75 | Multi | `NetC11_1` |
| R13 | 1889.8 | 1328.7 | 1.25 × 1.25 | 0.75 | Multi | `NetR13_1` |
| R13 | 2283.5 | 1328.7 | 1.25 × 1.25 | 0.75 | Multi | `NetC12_1` |
| R14 | 2972.6 | 2411.4 | 1.25 × 1.25 | 0.75 | Multi | `NetC12_1` |
| R14 | 2972.6 | 2805.1 | 1.25 × 1.25 | 0.75 | Multi | `BATT-` |
| S1 | 1889.8 | 1151.6 | 1.60 × 1.60 | 0.80 | Multi | `NetR13_1` |
| S1 | 3267.7 | 1151.6 | 1.60 × 1.60 | 0.80 | Multi | `NetC9_1` |
| U1 | 1924.4 | 1486.2 | 1.88 × 1.88 | 1.02 | Multi | `NetU1_JP7_1` |
| U1 | 1924.4 | 1586.2 | 1.88 × 1.88 | 1.02 | Multi | `NetR1_2` |
| U1 | 1924.4 | 1686.2 | 1.88 × 1.88 | 1.02 | Multi | `NetR9_2` |
| U1 | 1924.4 | 1786.2 | 1.88 × 1.88 | 1.02 | Multi | `NetR2_2` |
| U1 | 1924.4 | 1886.2 | 1.88 × 1.88 | 1.02 | Multi | `NetR5_1` |
| U1 | 1924.4 | 1986.2 | 1.88 × 1.88 | 1.02 | Multi | `NetR4_1` |
| U1 | 1924.4 | 2086.2 | 1.88 × 1.88 | 1.02 | Multi | `NetJ2_3` |
| U1 | 1924.4 | 2186.2 | 1.88 × 1.88 | 1.02 | Multi | `NetU1_JP7_8` |
| U1 | 1924.4 | 2286.2 | 1.88 × 1.88 | 1.02 | Multi | `GND` |
| U1 | 1924.4 | 2386.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U1 | 1924.4 | 2486.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U1 | 1924.4 | 2586.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U1 | 1974.4 | 2686.2 | 1.88 × 1.88 | 1.02 | Multi | `GND` |
| U1 | 2074.4 | 2686.2 | 1.88 × 1.88 | 1.02 | Multi | `GND` |
| U1 | 2174.4 | 2686.2 | 1.88 × 1.88 | 1.02 | Multi | `BATT+` |
| U1 | 2274.4 | 2686.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U1 | 2374.4 | 2686.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U1 | 2474.4 | 2686.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U1 | 2524.4 | 1486.2 | 1.88 × 1.88 | 1.02 | Multi | `NetU1_JP6_12` |
| U1 | 2524.4 | 1586.2 | 1.88 × 1.88 | 1.02 | Multi | `NetU1_JP6_11` |
| U1 | 2524.4 | 1686.2 | 1.88 × 1.88 | 1.02 | Multi | `NetU1_JP6_10` |
| U1 | 2524.4 | 1786.2 | 1.88 × 1.88 | 1.02 | Multi | `NetU1_JP6_9` |
| U1 | 2524.4 | 1886.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U1 | 2524.4 | 1986.2 | 1.88 × 1.88 | 1.02 | Multi | `NetR11_1` |
| U1 | 2524.4 | 2086.2 | 1.88 × 1.88 | 1.02 | Multi | `NetR3_2` |
| U1 | 2524.4 | 2186.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U1 | 2524.4 | 2286.2 | 1.88 × 1.88 | 1.02 | Multi | `BATT+` |
| U1 | 2524.4 | 2386.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U1 | 2524.4 | 2486.2 | 1.88 × 1.88 | 1.02 | Multi | `GND` |
| U1 | 2524.4 | 2586.2 | 1.88 × 1.88 | 1.02 | Multi | `—` |
| U2 | 3146.5 | 2426.4 | 1.68 × 1.68 | 1.12 | Multi | `NetC11_1` |
| U2 | 3146.5 | 2726.4 | 1.68 × 1.68 | 1.12 | Multi | `NetC9_1` |
| U2 | 3246.5 | 2426.4 | 1.68 × 1.68 | 1.12 | Multi | `NetU2_2` |
| U2 | 3246.5 | 2726.4 | 1.68 × 1.68 | 1.12 | Multi | `NetC9_1` |
| U2 | 3346.5 | 2426.4 | 1.68 × 1.68 | 1.12 | Multi | `NetC12_1` |
| U2 | 3346.5 | 2726.4 | 1.68 × 1.68 | 1.12 | Multi | `BATT-` |
| U2 | 3446.5 | 2426.4 | 1.68 × 1.68 | 1.12 | Multi | `NetC9_1` |
| U2 | 3446.5 | 2726.4 | 1.68 × 1.68 | 1.12 | Multi | `BATT-` |
| U2 | 3546.5 | 2426.4 | 1.68 × 1.68 | 1.12 | Multi | `NetR10_1` |
| U2 | 3546.5 | 2726.4 | 1.68 × 1.68 | 1.12 | Multi | `NetC9_1` |
| U2 | 3646.5 | 2426.4 | 1.68 × 1.68 | 1.12 | Multi | `NetU2_2` |
| U2 | 3646.5 | 2726.4 | 1.68 × 1.68 | 1.12 | Multi | `—` |
| U2 | 3746.5 | 2426.4 | 1.68 × 1.68 | 1.12 | Multi | `BATT-` |
| U2 | 3746.5 | 2726.4 | 1.68 × 1.68 | 1.12 | Multi | `—` |
| U3 | 2748.0 | 1386.2 | 1.70 × 1.70 | 1.02 | Multi | `GND` |
| U3 | 2748.0 | 1486.2 | 1.70 × 1.70 | 1.02 | Multi | `NetU1_JP6_12` |
| U3 | 2748.0 | 1586.2 | 1.70 × 1.70 | 1.02 | Multi | `NetU1_JP6_11` |
| U3 | 2748.0 | 1686.2 | 1.70 × 1.70 | 1.02 | Multi | `NetU1_JP6_10` |
| U3 | 2748.0 | 1786.2 | 1.70 × 1.70 | 1.02 | Multi | `NetU1_JP6_9` |
| U3 | 2748.0 | 1886.2 | 1.70 × 1.70 | 1.02 | Multi | `—` |
| U3 | 2748.0 | 1986.2 | 1.70 × 1.70 | 1.02 | Multi | `—` |
| U3 | 2748.0 | 2086.2 | 1.70 × 1.70 | 1.02 | Multi | `GND` |
| U3 | 3748.0 | 1386.2 | 1.70 × 1.70 | 1.02 | Multi | `GND` |
| U3 | 3748.0 | 1486.2 | 1.70 × 1.70 | 1.02 | Multi | `GND` |
| U3 | 3748.0 | 1586.2 | 1.70 × 1.70 | 1.02 | Multi | `BATT+` |
| U3 | 3748.0 | 1686.2 | 1.70 × 1.70 | 1.02 | Multi | `NetU1_JP7_1` |
| U3 | 3748.0 | 1786.2 | 1.70 × 1.70 | 1.02 | Multi | `NetU1_JP7_8` |
| U3 | 3748.0 | 1886.2 | 1.70 × 1.70 | 1.02 | Multi | `—` |
| U3 | 3748.0 | 1986.2 | 1.70 × 1.70 | 1.02 | Multi | `—` |
| U3 | 3748.0 | 2086.2 | 1.70 × 1.70 | 1.02 | Multi | `—` |

## 6. Copper routing by net

`GND` has zero tracks — it is carried entirely by the two polygons and 33 stitching vias.

| Net | Track widths (mil) | Top length (mil) | Bottom length (mil) | Total (mm) |
|---|---|---:|---:|---:|
| `BATT+` | 19.685 | 2282 | 7513 | 248.8 |
| `BATT-` | 19.685 | 0 | 4471 | 113.6 |
| `NetC11_1` | 19.685 | 0 | 1152 | 29.3 |
| `NetC12_1` | 19.685 | 1861 | 826 | 68.2 |
| `NetC2_1` | 19.685 | 0 | 982 | 24.9 |
| `NetC9_1` | 19.685 | 2310 | 2169 | 113.8 |
| `NetJ2_1` | 19.685 | 0 | 680 | 17.3 |
| `NetJ2_3` | 19.685 | 0 | 1916 | 48.7 |
| `NetJ3_3` | 19.685 | 0 | 523 | 13.3 |
| `NetJ3_4` | 19.685 | 0 | 269 | 6.8 |
| `NetLS1_P` | 19.685 | 0 | 623 | 15.8 |
| `NetQ1_1` | 19.685 | 0 | 1305 | 33.2 |
| `NetR10_1` | 19.685 | 0 | 507 | 12.9 |
| `NetR11_1` | 19.685 | 0 | 555 | 14.1 |
| `NetR13_1` | 19.685 | 0 | 177 | 4.5 |
| `NetR1_2` | 19.685 | 0 | 439 | 11.2 |
| `NetR2_2` | 19.685 | 0 | 813 | 20.7 |
| `NetR3_2` | 19.685 | 2541 | 0 | 64.6 |
| `NetR4_1` | 19.685 | 0 | 993 | 25.2 |
| `NetR5_1` | 19.685 | 0 | 241 | 6.1 |
| `NetR9_2` | 19.685 | 1032 | 0 | 26.2 |
| `NetU1_JP6_10` | 11.811 | 0 | 224 | 5.7 |
| `NetU1_JP6_11` | 11.811 | 0 | 224 | 5.7 |
| `NetU1_JP6_12` | 11.811 | 0 | 224 | 5.7 |
| `NetU1_JP6_9` | 11.811 | 0 | 224 | 5.7 |
| `NetU1_JP7_1` | 11.811 | 0 | 2457 | 62.4 |
| `NetU1_JP7_8` | 11.811 | 0 | 3120 | 79.3 |
| `NetU2_2` | 19.685 | 573 | 0 | 14.6 |

## 7. Decoupling proximity (pad-to-pad, mm)

| Supply pin | Nearest `BATT+` cap pad | Distance |
|---|---|---:|
| U3.3 (Ra-02 3.3 V) | C1 | 60.9 |
| U3.3 (Ra-02 3.3 V) | C3 | 35.1 |
| U3.3 (Ra-02 3.3 V) | C4 | 15.7 |
| U3.3 (Ra-02 3.3 V) | C5 | 51.2 |
| U3.3 (Ra-02 3.3 V) | C6 | 5.5 |
| U3.3 (Ra-02 3.3 V) | C7 | 13.0 |
| U3.3 (Ra-02 3.3 V) | C8 | 10.8 |
| U1 VCC (ATmega) | C1 | 38.8 |
| U1 VCC (ATmega) | C3 | 41.6 |
| U1 VCC (ATmega) | C4 | 40.2 |
| U1 VCC (ATmega) | C5 | 16.7 |
| U1 VCC (ATmega) | C6 | 40.7 |
| U1 VCC (ATmega) | C7 | 37.2 |
| U1 VCC (ATmega) | C8 | 41.7 |
| U2.14 (74HC74 VCC) | C9 | 26.3 |
| U2.14 (74HC74 VCC) | C10 | 6.8 |
