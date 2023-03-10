# Producer-Consumer-Problem

# Project description
Producer as a server, accepting incoming transactions and adding them to a queue of work
to be done. The consumers are each a thread looking for work from the server to execute. 

User specify the number of consumer by using the first input argument called "nthreads". 

Input commands either from file or keyboard. Two commands are:

  T<n> 
    Execute a transaction with integer parameter n, with n > 0. There is a routine
    that gets called for each transaction: void Trans(int n). The parameter n is
    used by Trans to determine how long to simulate the execution of the transaction.
    
  S<n> The sleep command simulates having the producer wait between receiving new
    transactions to process. The producer sleeps for a time determined by integer
    parameter n, with n taking on values 1..100. There is a routine that gets called for
    each sleep request: void Sleep(int n).
 
 Results will be printed on the file called "prodcon.<id>.log"
   
# How to run the program:
    Command: prodcon nthreads <id> < inputfile
    
    Note: id is optional and represents an integer to be used to differentiate the output files.
