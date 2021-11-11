# Euler III: Design

- [Individual Node](https://scicomp.ethz.ch/wiki/Euler#Euler_III)
  - Quad-Core Intel Xeon E3-1585Lv5 processor (3.0-3.7 GHz)
  - 32 GB of DDR4 memory clocked at 2133 MHz
  - [Server Blade](https://buy.hpe.com/us/en/moonshot-systems/moonshot-cartridges/proliant-server-cartridges/proliant-server-cartridges/hpe-proliant-m710x-server-blade/p/1009011712)
- Chassis
  - Contains 45 nodes
  - Each node connected via 10G Ethernet to internal switch
- Euler
  - Contains 27 chassis
  - Each chassis connected via 40G Ethernet to one of the core switches
- Network
  - Mellanox Ethernet (same chip as InfiniBand)

<details>
<summary>Original E-Mail</summary>

```
Dear Elwin,

Euler III is based on HPE's Moonshot system [1]. A Moonshot chassis contains 45 nodes [2] connected via 10G Ethernet to
one or two internal 10G/40G switches (only one in our case). Each chassis is then connected via 40G Ethernet (4 links,
if my memory is correct, so 160G in total) to one of the core switches of Euler. From a topological point of view, you
could say that one Moonshot chassis is equivalent to one rack in a traditional cluster -- in 1/10th of the space!

Although Moonshot was not originally designed for HPC, Euler III proved that its small (4 cores) fast CPUs and
low-latency network (Mellanox Ethernet, using the same chips as InfiniBand) was perfect for massively parallel
computation. Back in 2017, HPE used Euler III as a showcase for huge fluid dynamics simulations (Ansys FLUENT with >1
billion cells). Its performance was so good that its design was adopted by a majority of Formula 1 teams :-)

Cheers, Olivier

[1] https://buy.hpe.com/us/en/moonshot-systems/moonshot-chassis/moonshot-chassis/moonshot-chassis/hpe-moonshot-1500-chassis/p/5365577

[2] https://buy.hpe.com/us/en/moonshot-systems/moonshot-cartridges/proliant-server-cartridges/proliant-server-cartridges/hpe-proliant-m710x-server-blade/p/1009011712
```
</details>