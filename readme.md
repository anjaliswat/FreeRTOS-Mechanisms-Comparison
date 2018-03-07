PROJECT DESCRIPTION :

For this project, the operating system being used is FreeRTOS. FreeRTOS was run in an Xilinx Zynq SOC. In order to aid us in debug, a UART interface has also been added to communicate with the FreeRTOS OS. The SW development is done within Xilinx SDK Development environment.

The goal of this project was to see the impact of Synchronization overhead of different mechanisms into program runtime. In this project, the evaluation of the following synchronization mechanisms has been done : 
- Semaphores
- Reference Coloring Algortihm 
- FreeRTOS Task Notifications

In order to do the evaluation, a multi-threaded based program that has RAW
and WAR dependencies has been constructed, similar to the example that will run inside the FreeRTOS platform. The
programs would have a similar structure but will differ in the use of each synchronization
mechanism.

Their performance has been evaluated on the following parameters:
- Ensure that no RAW/WAR violations have occurred – data integrity
- Qualitatively compare and contrast the overall latency between several implementation –
thereby extrapolating the synchronization overhead between different mechanisms.
- If possible, measure the memory footprint of the different mechanisms


RESULTS :

Before starting the tests, we were expecting the Semaphore implementation to have the worst performance and for the task notifications to have the best performance. However, once we initially started the tests, we noticed that the as we decrease the sync interval for Semaphores, it performed faster than the Event groups. After some quick debugging, we then realized the
function calls for the event groups implementation also has a parameter for the Sync interval. After setting a the same Sync Interval parameter between the Semaphore and the Event group, we were able to see our expected results.

Based on the FreeRTOS online material, we should expect around 46 % performance improvements in Direct Task Notification based implementation compared to Semaphore based implementation. We are seeing a 42.55% which is close to the documented results.

