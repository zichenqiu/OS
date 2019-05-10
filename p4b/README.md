CS 537 Spring 2019 
Project 4b: kernel threads

This project is about adding real kernel threads to xv6. Specifically, there are three things. First, we define a new system call to create a kernel thread, called clone(), as well as one to wait for a thread called join(). Then we use clone() to build a little thread library, with a thread_create() call and lock_acquire() and lock(release() functions.


For more details about this project, click here: http://pages.cs.wisc.edu/~shivaram/cs537-sp19/p4b.html
