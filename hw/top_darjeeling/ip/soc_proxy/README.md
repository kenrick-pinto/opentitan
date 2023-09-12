# SoC Proxy Technical Specification

SoC Proxy is a simple comportable IP module that facilitates the breakout of signals and buses internal to OpenTitan (similar to Sensor Control for AST).

## Comportable Interfaces

- Clock / reset (intended for connection to the fast clock domain)
- 1 TL-UL Device port for comportable CSR node
- 1 TL-UL Device port with an address range to egress into CTN
- 1 Fatal alert for bus integrity
- 8 Fatal external alert channels
- 8 Recoverable external alert channels
- 2 Wakeup requests
  - Internal wakeup request, should be asserted whenever an external alert or IRQ is seen
  - External wakeup request
- 4 Interrupts
  - External interrupt requests

## SoC-facing interfaces

- Synchronous interfaces:
  - 1 TL-UL Host port for egress into CTN
  - 8 Differential alert input signals with
    - Status of each alert can be read and acknowledged via CSRs
    - Severity (fatal or recoverable is programmable in the same way as in Sensor Control)
    - Optional acknowledgment signals (as in Sensor Control)
    - The main difference with respect to Sensor Control is that each alert is sent out via its own alert channel instead of aggregating, since that way the alert crash dump latched in Reset Manager provides more info than if the alerts were aggregated.
- Asynchronous interfaces:
  - 1 Wakeup request, asynchronous and level-encoded
  - 4 Interrupts, asynchronous and level-encoded