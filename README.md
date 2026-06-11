# SoftwarePnPPowerDriverTemplate
software driver that handles pnp and power irps

# Installation
- Copy .inf and .sys file to the windows VM
- Use pnputil to install the driver
- `pnputil /add-driver SoftwarePnPPower.inf /install /force`
- you can use devcon.exe for installation as well, `devcon.exe install softwarepnppower.inf ROOT\SOFTWAREPNPPOWER`
- you can use device manager and install the .inf file
  
