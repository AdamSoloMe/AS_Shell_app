#include <stdio.h>
#include <stdlib.h>  //Library for exit call
#include <pthread.h> //This is the library for threads
#include <sys/types.h>
#include <sys/wait.h> //this is the library for waitpid
#include <ctype.h>    //this is the library for the isspace function in my trim function
#include <string.h>
#include <unistd.h> //library for project error message


char *commandlinefilepathsforUnix[256] = {"/bin", NULL}; // this will store the intial file directory since all the commands will be under /bin (in project instructions)
FILE *fp = NULL;
char *storedcommandline = NULL;

void run_command_line_functions(char *args[], int args_num, FILE *out);//this function used for executing the commands 

void printErrorMessage()
{ // this function will print an error in case anything in the shell is inputted incorrectly
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

struct commandline_arg_node
{ // this linked list will be used to store and parse each command the user inputs into the shell
  pthread_t commandthread;
  char *storedcommandlinetokens;
};


char *trim(char *commandline_space)
{ // this trim function is used for removing/processing spaces within the commandline input
  // remove spaces in/from the front
  while (isspace(*commandline_space)){
    commandline_space++;
  }

  if (*commandline_space == '\0'){
    return commandline_space; // return if the string is empty
  }
  // remove spaces in/from the back
  char *end = commandline_space + strlen(commandline_space) - 1;
  while (end > commandline_space && isspace(*end)){
    end--;
  }
  end[1] = '\0';
  return commandline_space;
}
int locateCommand(char path[], char *commandArg)
{ // this function will search for the file path of the associated Unix command file to be executed
  int i = 0;
  while (commandlinefilepathsforUnix[i] != NULL)
  {
    snprintf(path, 256, "%s/%s", commandlinefilepathsforUnix[i],commandArg);
    if (access(path, X_OK) == 0){
      return 0;
    }
    i++;
  }
  return -1;
}

void Memory_cleaner(void)
{
  free(storedcommandline);
  fclose(fp);
}



void *getCommandlineInput(void *commandlineInput)
{
  char *cmdlnargs[256];
  struct commandline_arg_node *commandline_args;
  FILE *output = stdout;
  commandline_args = (struct commandline_arg_node *)commandlineInput;
  size_t num_of_cmdlnargs = 0;
  char *commandLine = commandline_args->storedcommandlinetokens;

  char *command = strsep(&commandLine, ">");
  if (command == NULL || *command == '\0')
  {
    printErrorMessage();
    return NULL;
  }

  command = trim(command);

  if (commandLine != NULL)
  {

    if ((output = fopen(trim(commandLine), "w")) == NULL)
    {
      printErrorMessage();
      return NULL;
    }
  }

  char **cmdlnp = cmdlnargs;
  while ((*cmdlnp = strsep(&command, " \t")) != NULL)
    if (**cmdlnp != '\0')
    {
      *cmdlnp = trim(*cmdlnp);
      cmdlnp++;
      if (++num_of_cmdlnargs >= 256)
        break;
    }

  if (num_of_cmdlnargs > 0)
    run_command_line_functions(cmdlnargs, num_of_cmdlnargs, output);

  return NULL;
}

void redirectCommandLineOutput(FILE *out)
{
  int outFileLocation;
  if ((outFileLocation = fileno(out)) == -1)
  {
    printErrorMessage();
    return;
  }

  if (outFileLocation != STDOUT_FILENO)
  {
    // here the output will be from the sent from the terminal output to  be redirected to the output file
    if (dup2(outFileLocation, STDOUT_FILENO) == -1)
    {
      printErrorMessage();
      return;
    }
    fclose(out);
  }
}
void run_command_line_functions(char *args[], int args_num, FILE *out)
{
  // these if statements will check to make sure the default built-in commands will run first
  if (strcmp(args[0], "exit") == 0)
  {
    if (args_num > 1)
      printErrorMessage();
    else
    {
      atexit(Memory_cleaner);
      exit(0);
    }
  }
  else if (strcmp(args[0], "cd") == 0)
  {
    if (args_num == 1 || args_num > 2)
      printErrorMessage();
    else if (chdir(args[1]) == -1)
      printErrorMessage();
  }
  else if (strcmp(args[0], "path") == 0)
  {
    size_t i = 0;
    commandlinefilepathsforUnix[0] = NULL;
    for (; i < args_num - 1; i++){
      commandlinefilepathsforUnix[i] = strdup(args[i + 1]);
    }

    commandlinefilepathsforUnix[i + 1] = NULL;
  }
  else
  {
    // this is where the commands which are not bulit in will be checked
    char path[256];
    if (locateCommand(path, args[0]) == 0)
    {
      pid_t pid = fork();
      if (pid == -1){
        printErrorMessage();
      }
      else if (pid == 0)
      {
        // child process
        redirectCommandLineOutput(out);

        if (execv(path, args) == -1){
          printErrorMessage();
        }
      }
      else{
        waitpid(pid, NULL, 0); // parent process waits child
      }
    }
    else{
      printErrorMessage(); // print error if the filepath/command was not found
    }
  }
}
int main(int argc, char *argv[])
{
  
  int Shell_state; // this will determine the state of the shell based on the commandline
  fp = stdin;      // this will read in the batch file of commands
  int Interactive_mode = 1;
  ssize_t commandlinebuffervalue;
  int Batch_mode = 2;
  

  size_t commandlinecap = 0;

 
  Shell_state = Interactive_mode; // by default the program is in interactive mode
  if (argc > 1)
  { // this statement will ensure that if the user calls the shell without arguements then the shell will be batch mode
    Shell_state = Batch_mode;
    if (argc > 2 || (fp = fopen(argv[1], "r")) == NULL)
    {
      printErrorMessage();
      exit(1); // I put this here to ensure that only the error message specified in the project instructions will printed
      // if the there are more than 2 arguments given in batch mode or the batch file is not found
      // otherwise I kept getting a differnet error message which was a segementation fault
    }
  }

  while (2)
  { // this ensures that the while loop will run continously to get the User's input on the commandline in the shell
    if (Shell_state == Interactive_mode)
    {
      printf("wish> ");
    }

    if ((commandlinebuffervalue = getline(&storedcommandline, &commandlinecap, fp)) > 0)
    {
      char *tokens;
      int number_of_commands = 0;
      struct commandline_arg_node clargs[256];

      // extra test cases
      if (storedcommandline[commandlinebuffervalue - 1] == '\n')
      {                                                       // this statement checks/makes sure that there is no extra newline character being processed in the shell
        storedcommandline[commandlinebuffervalue - 1] = '\0'; // if there is a newline character the shell will replace the character indicating a newline with \0 to indicate the end the command
      }
      char *Multicommandtempbuffer = storedcommandline;

      // for mulitple commands in a line
      while ((tokens = strsep(&Multicommandtempbuffer, "&")) != NULL)
        if (tokens[0] != '\0')
        { // while the command array is not equal to empty string
          clargs[number_of_commands++].storedcommandlinetokens = strdup(tokens);
          if (number_of_commands >= 256)
          { // if the number of commands exceeds the buffer size the loop will break
            break;
          }
        }

      for (size_t i = 0; i < number_of_commands; i++){ // this for loop will create a process to execute each command the user entered into the shell
        if (pthread_create(&clargs[i].commandthread, NULL, &getCommandlineInput, &clargs[i]) != 0)
        { // if the thread for the command is unable to be created the program will print an error
          printErrorMessage();
        }
      }

      for (size_t i = 0; i < number_of_commands; i++)
      {
        if (pthread_join(clargs[i].commandthread, NULL) != 0)
        {
          printErrorMessage();
        }
        if (clargs[i].storedcommandlinetokens != NULL)
        {
          free(clargs[i].storedcommandlinetokens);
        }
      }
    }
    else if (feof(fp) != 0)
    {
      atexit(Memory_cleaner);
      exit(0); // EOF
    }
  }

  return 0;
}
