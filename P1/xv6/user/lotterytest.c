// Experimental evaluation of the lottery scheduler.
//
// Runs three CPU-bound children holding 30, 20 and 10 tickets (3:2:1)
// and samples their accumulated ticks with getpinfo() at fixed
// intervals.  Output is CSV so it can be pasted into a spreadsheet.
//
// Expected long-run CPU shares: 30/60 = 50%, 20/60 = 33.3%, 10/60 = 16.7%.
//
// Usage: lotterytest [samples [interval]]
//   samples   number of measurements to take   (default 30)
//   interval  timer ticks between measurements (default 100)

#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

#define NCHILD 3

int tickets[NCHILD] = { 30, 20, 10 };

// Burn CPU forever.  The loop never sleeps or does I/O, so every
// time slice the child wins is used in full.
void
spin(void)
{
  for(;;)
    ;
}

// Return the accumulated ticks of process pid, or -1 if not found.
int
ticksof(struct pstat *st, int pid)
{
  int i;

  for(i = 0; i < NPROC; i++)
    if(st->inuse[i] && st->pid[i] == pid)
      return st->ticks[i];
  return -1;
}

int
main(int argc, char *argv[])
{
  struct pstat st;
  int pids[NCHILD];
  int t[NCHILD];
  int samples, interval, start, total, tsum, obs, exp, i, s;

  samples = 30;
  interval = 100;
  if(argc > 1)
    samples = atoi(argv[1]);
  if(argc > 2)
    interval = atoi(argv[2]);
  if(samples < 1 || interval < 1){
    printf(2, "usage: lotterytest [samples [interval]]\n");
    exit();
  }

  // Start the children.  A child inherits its parent's tickets at
  // fork(), so the parent sets the count before each fork and every
  // child holds its intended tickets from the moment it exists.
  // (If the child set them itself, it would hold the inherited count
  // until it first got scheduled, which skews the early samples.)
  for(i = 0; i < NCHILD; i++){
    if(settickets(tickets[i]) < 0){
      printf(2, "lotterytest: settickets failed\n");
      while(--i >= 0)
        kill(pids[i]);
      while(wait() > 0)
        ;
      exit();
    }
    pids[i] = fork();
    if(pids[i] < 0){
      printf(2, "lotterytest: fork failed\n");
      while(--i >= 0)
        kill(pids[i]);
      while(wait() > 0)
        ;
      exit();
    }
    if(pids[i] == 0)
      spin();
  }

  // Give the parent many tickets so it gets the CPU right away each
  // time it wakes up to take a sample.  It sleeps the rest of the
  // time, and sleeping processes are not in the lottery, so this
  // does not change the children's shares.
  settickets(100000);

  printf(1, "pids: %d (%d tickets), %d (%d tickets), %d (%d tickets)\n",
         pids[0], tickets[0], pids[1], tickets[1], pids[2], tickets[2]);
  printf(1, "time,ticks_%d,ticks_%d,ticks_%d\n",
         tickets[0], tickets[1], tickets[2]);

  start = uptime();
  for(s = 0; s < samples; s++){
    sleep(interval);
    if(getpinfo(&st) < 0){
      printf(2, "lotterytest: getpinfo failed\n");
      break;
    }
    for(i = 0; i < NCHILD; i++)
      t[i] = ticksof(&st, pids[i]);
    printf(1, "%d,%d,%d,%d\n", uptime() - start, t[0], t[1], t[2]);
  }

  // Summary of the last sample against the expected shares.
  // User printf has no %f, so shares are computed in tenths of a percent.
  total = t[0] + t[1] + t[2];
  tsum = tickets[0] + tickets[1] + tickets[2];
  if(total > 0){
    for(i = 0; i < NCHILD; i++){
      obs = t[i] * 1000 / total;
      exp = tickets[i] * 1000 / tsum;
      printf(1, "%d tickets: observed %d.%d%%, expected %d.%d%%\n",
             tickets[i], obs / 10, obs % 10, exp / 10, exp % 10);
    }
  }

  for(i = 0; i < NCHILD; i++)
    kill(pids[i]);
  for(i = 0; i < NCHILD; i++)
    wait();
  exit();
}
