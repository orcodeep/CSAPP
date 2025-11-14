Good Reference for Binary and Hex representations and conversion b/w them

<pre>
H̲E̲X̲ C̲A̲L̲C̲U̲L̲A̲T̲I̲O̲N̲:

D 698 H ?

16| 698 |43
   -64
   -----
     58
    -48
    ----
     10

16| 43 |2
   -32
   ----
    11

698 ÷ 16 = 43 remainder 10
43  ÷ 16 = 2  remainder 11

combine digits: 2 → 11 → 10 
        In Hex: 2   B    A

D 698 H 0x2BA
check:
2 × 16² + 11 × 16¹ + 10 × 16⁰ = 698
___________________________________

D 339764 H ?

339764 ÷ 16 = 21235 remainder 4
21235  ÷ 16 = 1327  remainder 3
1327   ÷ 16 = 82    remainder 15
82     ÷ 16 = 5     remainder 2

combine digts: 5 → 2 → 15 → 3 → 4
       In Hex: 5   2   F    3   4

D 339764 H 0x52F34
check:
5 × 16⁴ + 2 × 16³ + 15 × 16² + 3 × 16¹ + 4 × 16⁰ = 339764

</pre>