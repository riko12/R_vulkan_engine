# Vulkan tutorial #
To use Vulkan in your application it is important to install it on the machine.
The runtime is a library which should be linked to your application.
I'm doing it, for example (ubuntu 18.04 machine),  like this:
```
target_link_libraries(tut04 vulkan
```  
Currently VulkanSDK used in this tutorial is version 1.2  
As known, Vulkan is an low level generic API software to achive performance and efficiency of the graphic card independently from hardware. 
Mean target of this tutorial is understanding of the architecture and methodology to build a graphic engine, vulkan based.
This tutorial, is build in the form of steps, which passing through as described below.

**To build all steps**

```
cd build
cmake .. -Wno-dev
make
```
or after at least one time **cmake**-call, **make tutx** where the x is representing the proper step number and tutx is the proper target witch you would like to build for example: 
```
make tut01
```

**Step 00**
This application will search all the physical gr-card contained in the machine
start:
```
tut00  
```

**Step 01**
This application will search all the physical gr-card contained in the machine
start:
```
tut01  
```

**Step 02**
This application will search all the physical gr-card contained in the machine
start:
```
tut02  
```

**Step 03**
This application will search all the physical gr-card contained in the machine
start:
```
tut03  
```

**Step 04**
This application will search all the physical gr-card contained in the machine
start:
```
tut04  
```

**Step 05**
This application will search all the physical gr-card contained in the machine  
start:
```
tut05  
```

**Step 06**
This application will search all the physical gr-card contained in the machine  
start:
```
tut06  
```

**Step 07**
This application will search all the physical gr-card contained in the machine  
start:
```
tut07  
```

**Step 08**
This application will search all the physical gr-card contained in the machine  
start:
```
tut08  
```


