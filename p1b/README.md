Zichen Qiu
CS-537 Spring 2019
Project 1b


The goal of this project is to add a simple system call to xv6, getopenedcount(), which returns how many times that the open() system call has been called by user processes since the time that the kernel was booted.



To achieving this, I simply add a counter (openedcount) that  will increment by 1 each time the open() system call is called. My new system call is a function int getopenedcount(void) that returns the value of the counter.



To execute, go to the xv6 directory and type make, and then type

make qemu-nox 

this will successfully run xv6, where you will need to simply type 

1) test0 (testing file provided by TA) and 

2) tester2 (my own testin file)


To find more information about this assginemnt, you can click the following link
http://pages.cs.wisc.edu/~shivaram/cs537-sp19/p1b.html

