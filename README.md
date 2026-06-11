# SoftwarePnPPowerDriverTemplate
software driver that handles pnp and power irps

sorry for the mess of the files, i tried to push from visual studio but got errors

# Installation
- Copy .inf and .sys file to the windows VM
- Use pnputil to install the driver
- `pnputil /add-driver SoftwarePnPPower.inf /install /force`
- you can use devcon.exe for installation as well, `devcon.exe install softwarepnppower.inf ROOT\SOFTWAREPNPPOWER`
- you can use device manager and install the .inf file
  
# Note
My code is not perfect 😭, any bug fixes are welcome, use at your own risk of bsod 😭

# DebugView screenshots
![ Installing/disabling/enabling driver ] (images/debugview1.png)
