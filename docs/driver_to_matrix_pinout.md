# 1088AS 8x8 LED Matrix Pinout & Activation Reference

This document is the **electrical source of truth** for the 1088AS 8x8 LED matrix displays used in the MakiNeat system.

---

## 1. Physical Orientation & Pin Layout

The matrix is viewed from the **front display face**. The notch and the `1088AS` silkscreen label are located at the **bottom edge**.

Pins are named using single hexadecimal characters **`0` through `F`**:
- **Top Row (Pins 0 to 7)**: Opposite the notch, ordered left-to-right (Col 0 to Col 7).
- **Bottom Row (Pins 8 to F)**: On the notch side, ordered left-to-right (Col 0 to Col 7).

```
       ----------------------------             
Row 0 |   0  1  2  3  4  5  6  7   |            
      |   .  .  .  .  .  .  .  .   |            
      |                            |            
      |                            |            
      |                            |            
      |                            |            
      |                            |            
      |                            |            
      |   8  9  A  B  C  D  E  F   |            
      |   .  .  .  .  .  .  .  .   |            
Row 7 |                            |            
       ------------/  \-------------            
                  (Notch)
       Col 0                   Col 7   
```

---

## 2. Electrical Operation (Anodes vs. Cathodes)

To light a pixel at coordinate **(Row $r$, Col $c$)**:
- **Positive Voltage ($+V$)** is applied to the **Column Pin (Anode)**.
- **Ground ($\text{GND}$)** is applied to the **Row Pin (Cathode)**.

### Column Pins (Anodes: Apply $+V$)
| Display Column | Position | Matrix Pin (0-F) |
|:--------------:|:--------:|:----------------:|
| **Col 0** | Leftmost (bit 7) | **3** |
| **Col 1** | bit 6 | **A** |
| **Col 2** | bit 5 | **B** |
| **Col 3** | bit 4 | **6** |
| **Col 4** | bit 3 | **D** |
| **Col 5** | bit 2 | **5** |
| **Col 6** | bit 1 | **1** |
| **Col 7** | Rightmost (bit 0) | **0** |

### Row Pins (Cathodes: Pull to $\text{GND}$)
| Display Row | Position | Matrix Pin (0-F) |
|:-----------:|:--------:|:----------------:|
| **Row 0** | Topmost | **7** |
| **Row 1** | line 1 | **2** |
| **Row 2** | line 2 | **F** |
| **Row 3** | line 3 | **4** |
| **Row 4** | line 4 | **8** |
| **Row 5** | line 5 | **E** |
| **Row 6** | line 6 | **9** |
| **Row 7** | Bottommost | **C** |

---

## 3. Complete Pin Function List (0 to F)

| Pin | Type | Function | Activation Signal |
|:---:|:----:|:--------:|:-----------------:|
| **0** | Anode ($+$) | Column 7 | Source $+V$ |
| **1** | Anode ($+$) | Column 6 | Source $+V$ |
| **2** | Cathode ($-$) | Row 1 | Sink to GND |
| **3** | Anode ($+$) | Column 0 | Source $+V$ |
| **4** | Cathode ($-$) | Row 3 | Sink to GND |
| **5** | Anode ($+$) | Column 5 | Source $+V$ |
| **6** | Anode ($+$) | Column 3 | Source $+V$ |
| **7** | Cathode ($-$) | Row 0 | Sink to GND |
| **8** | Cathode ($-$) | Row 4 | Sink to GND |
| **9** | Cathode ($-$) | Row 6 | Sink to GND |
| **A** | Anode ($+$) | Column 1 | Source $+V$ |
| **B** | Anode ($+$) | Column 2 | Source $+V$ |
| **C** | Cathode ($-$) | Row 7 | Sink to GND |
| **D** | Anode ($+$) | Column 4 | Source $+V$ |
| **E** | Cathode ($-$) | Row 5 | Sink to GND |
| **F** | Cathode ($-$) | Row 2 | Sink to GND |

---

## 4. 8x8 LED Grid Matrix: Pin Pairs to Light Each LED

Format below: `(+Anode Pin / -Cathode Pin)`

| | **Col 0** (+Pin 3) | **Col 1** (+Pin A) | **Col 2** (+Pin B) | **Col 3** (+Pin 6) | **Col 4** (+Pin D) | **Col 5** (+Pin 5) | **Col 6** (+Pin 1) | **Col 7** (+Pin 0) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Row 0** (-Pin 7) | `+3 / -7` | `+A / -7` | `+B / -7` | `+6 / -7` | `+D / -7` | `+5 / -7` | `+1 / -7` | `+0 / -7` |
| **Row 1** (-Pin 2) | `+3 / -2` | `+A / -2` | `+B / -2` | `+6 / -2` | `+D / -2` | `+5 / -2` | `+1 / -2` | `+0 / -2` |
| **Row 2** (-Pin F) | `+3 / -F` | `+A / -F` | `+B / -F` | `+6 / -F` | `+D / -F` | `+5 / -F` | `+1 / -F` | `+0 / -F` |
| **Row 3** (-Pin 4) | `+3 / -4` | `+A / -4` | `+B / -4` | `+6 / -4` | `+D / -4` | `+5 / -4` | `+1 / -4` | `+0 / -4` |
| **Row 4** (-Pin 8) | `+3 / -8` | `+A / -8` | `+B / -8` | `+6 / -8` | `+D / -8` | `+5 / -8` | `+1 / -8` | `+0 / -8` |
| **Row 5** (-Pin E) | `+3 / -E` | `+A / -E` | `+B / -E` | `+6 / -E` | `+D / -E` | `+5 / -E` | `+1 / -E` | `+0 / -E` |
| **Row 6** (-Pin 9) | `+3 / -9` | `+A / -9` | `+B / -9` | `+6 / -9` | `+D / -9` | `+5 / -9` | `+1 / -9` | `+0 / -9` |
| **Row 7** (-Pin C) | `+3 / -C` | `+A / -C` | `+B / -C` | `+6 / -C` | `+D / -C` | `+5 / -C` | `+1 / -C` | `+0 / -C` |

---

## 5. MAX7219 Driver Wiring Reference

The MAX7219 is a multiplexed Common-Cathode driver:
- **Segments (`SEGDP`, `SEGA`-`SEGG`)** are current sources -> Connect to **Column Anodes** (`PIN 0`, `1`, `3`, `5`, `6`, `A`, `B`, `D`).
- **Digits (`DIG0`-`DIG7`)** are current sinks -> Connect to **Row Cathodes** (`PIN 2`, `4`, `7`, `8`, `9`, `C`, `E`, `F`).

```
               +----------------+
           DIN | 1           24 | DOUT
         PIN 7 | 2           23 | PIN D
         PIN 8 | 3           22 | PIN 3
           GND | 4           21 | PIN 5
         PIN 9 | 5           20 | PIN 6
         PIN F | 6  MAX7219  19 | V+
         PIN 4 | 7           18 | ISET
         PIN C | 8           17 | PIN 0
           GND | 9           16 | PIN B
         PIN E | 10          15 | PIN 1
         PIN 2 | 11          14 | PIN A
     LOAD (CS) | 12          13 | CLK
               +----------------+
```

*(Note: Matrix pins `PIN 0` through `PIN F` correspond to nets `DSn_0` through `DSn_F` in the system netlist).*

| Display Signal | MAX7219 Function | MAX7219 Pin | Matrix Pin | Net Name (`DSn`) |
|:--------------:|:----------------:|:------------:|:----------:|:----------------:|
| **Col 0** (MSB) | `SEGDP` | 22 | **3** | `DSn_3` |
| **Col 1** | `SEGA` | 14 | **A** | `DSn_A` |
| **Col 2** | `SEGB` | 16 | **B** | `DSn_B` |
| **Col 3** | `SEGC` | 20 | **6** | `DSn_6` |
| **Col 4** | `SEGD` | 23 | **D** | `DSn_D` |
| **Col 5** | `SEGE` | 21 | **5** | `DSn_5` |
| **Col 6** | `SEGF` | 15 | **1** | `DSn_1` |
| **Col 7** (LSB) | `SEGG` | 17 | **0** | `DSn_0` |
| **Row 0** (Top) | `DIG0` | 2 | **7** | `DSn_7` |
| **Row 1** | `DIG1` | 11 | **2** | `DSn_2` |
| **Row 2** | `DIG2` | 6 | **F** | `DSn_F` |
| **Row 3** | `DIG3` | 7 | **4** | `DSn_4` |
| **Row 4** | `DIG4` | 3 | **8** | `DSn_8` |
| **Row 5** | `DIG5` | 10 | **E** | `DSn_E` |
| **Row 6** | `DIG6` | 5 | **9** | `DSn_9` |
| **Row 7** | `DIG7` | 8 | **C** | `DSn_C` |
