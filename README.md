# Heat-Simulation-through-Parallal-Computing
Heat Simulation, also known as Thermal Simulation, refers to the simulation of the 
process of heat transfer, including how the distribution of heat on a specific piece of material, 
or that of a room, changes with time. 

## Contents of this Repository 
In recent times, a number of modeling and simulation measures have been applied to the 
analysis of heat transfer. There are various mechanisms of heat transfer, including heat 
conduction, convection, thermal radiation, and transfer of energy with phase changes. In 
spite of all the glamorous techniques and the complex classifications of heat transfer, this 
repository focuses on a rather simple simulation of heat conduction confined to a two-dimensional room with walls (Figure 1). Five programs (one sequantial and four parallel) have been written for the purpose to visualize the heat simulation process. Several factors will have an impact on the performance of these programs, typically includes the input size and the number of processors in parallel computing.

![image](https://user-images.githubusercontent.com/64362092/196092504-a4008670-0693-4c18-9b28-aaaf3acc2d07.png)

##### Figure 1. Heat Simulation within a Room with Walls and a Fireplace


## Task Distribution among Worker processors
For the parallal programs, we adopt the following task distribution strategy: each row of data (i.e. temperature) is distributed to one worker processor (Figure 2). 


![image](https://user-images.githubusercontent.com/64362092/196094211-bea1e08e-8648-4a06-b3d3-5b1f9047313d.png)

##### Figure 2. Distribution of Tasks in the MPI Program for Heat Simulation

## Communication between Worker Processors
Within the ongoing computation loop, the communications between worker processors are done in the following way (Figure 3). Each processor will communicate with the previous processor and the last processor, except for the first and the last processor. These intra-communications enable the temperature data to be commuted around its nearby processors, thus enable the computations of spreading heat.

![image](https://user-images.githubusercontent.com/64362092/196094194-237b7c3c-6378-43f7-92d7-942a33417ebd.png)

##### Figure 3. Communication between 4 Worker Processors in the Computation Loop 

