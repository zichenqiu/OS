Zichen Qiu
CS537- Spring 2019
Project P2b


In this project, I implemented a simplified multi-level feedback queue (MLFQ) schedule in xv6. The MLFQ scheduler has four priority queues: the top queue has the highest priority and the bottom queue has the lowest priority. WHen a process uses up its time-slice (counted as a number of ticks), it should be downgraded to the next lower priority level. The time-slice for higher priorities will be shorted than lower priorities. The scheduling method in each of the se queues will be round-robin, except the bottom queue which will be implemented as FIFO.

In addition, I also create one systen call for this project: int getpinfo(struct pstat *). Because my MLFQ implementaions are all in the kernel level, I need to extract useful information for each process by creating this system call so as to better test whether the implementation works as expected.

To run the xv6 environment, use make qeumu-nox. Doing so avoids the use of X windows and is generally fast and easy. To quit, you can type control-a followed by x to exit the emulation.


For more information about this project, click via this link: http://pages.cs.wisc.edu/~shivaram/cs537-sp19/p2b.html
