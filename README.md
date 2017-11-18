# CoolCooler

Get a cheap 12V cooler and pimp it with this Arduino project. Features:
- Low Battery Protection
- Configurable Duty cycle
- Internal/External temperature monitoring
- LED door light

Low Battery protection
----------------------
Can be choosen between:
- Always On
- 12.5V "Battery Ok": So battery will not fully discharg
- 13.0V "Safe Start": A battery in good contion would be able to start with this voltage.
- 13.8V "Engine On": Battery voltage stays at 13.8v under load when it is actively being charged by the engine.

NOTE: Voltages will vary depending on the car, generator, etc. These are failr estimations.

Configurable Duty Cycle
-----------------------
- Target 5C: Runs until internal temperature drops below 5C.
- Target 8C: same but for 8C.
- 25% Couple, 50% Fan: Runs a cycle of approx. 10 minutes, the thermocouple is active 25% of the time, and the fan 50%.
- 50% Couple, 75% Fan: like previous.
- 75% Couple, 100% fan: like previous too.
- Always On: like its basic functionality.
- Eco: Targets to be under 10C but runs more if internal temperature is too far of it.

Pending
-------
- Pictures
- Be better at estimation of real voltage of the battery (i.e. when the thermocouple is active, the voltage drops below the safety threshold, therefore the thermocouple is disengaged, this makes the voltage to raise, and start the thermocouple again... it should have a delay of a few minutes to let the battery recover or at least fluctuate every 15 minutes or more)
- Layouts
- Pictures
