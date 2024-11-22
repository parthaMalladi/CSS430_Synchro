#ifndef SHOP_H_
#define SHOP_H_
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <string>
#include <queue>
#include <unordered_map>
using namespace std;

#define kDefaultNumChairs 3
#define kDefaultNumBarbers 1

class Shop
{
public:
   Shop(int barbers, int num_chairs) : 
      max_waiting_cust_(num_chairs),
      max_barbers_(barbers),
      customer_in_chair_(barbers, 0),
      in_service_(barbers, false),
      money_paid_(barbers, false),
      cust_drops_(0),
      cond_customer_served_(barbers),
      cond_barber_paid_(barbers),
      cond_barber_sleeping_(barbers)
   { 
      init(); 
   };
   Shop() : 
      max_waiting_cust_(kDefaultNumChairs),
      max_barbers_(kDefaultNumBarbers),
      customer_in_chair_(kDefaultNumBarbers, 0),
      in_service_(kDefaultNumBarbers, false),
      money_paid_(kDefaultNumBarbers, false),
      cust_drops_(0),
      cond_customer_served_(kDefaultNumBarbers),
      cond_barber_paid_(kDefaultNumBarbers),
      cond_barber_sleeping_(kDefaultNumBarbers)
   { 
      init(); 
   };

   int visitShop(int id);   // return true only when a customer got a service
   void leaveShop(int id, int barber_id);
   void helloCustomer(int barber_id);
   void byeCustomer(int barber_id);
   int get_cust_drops() const;

 private:
   const int max_waiting_cust_;              // max number of threads that can wait
   const int max_barbers_;                   // number of available barbers
   queue<int> waiting_chairs_;               // queue of customers in the waiting chairs
   queue<int> available_barbers_;            // queue of barbers ready to get to work

   vector<int> customer_in_chair_;           // vector of customer IDs that are being serviced by index barber
   vector<bool> in_service_;                 // vector to keep track of customer and if they are in service
   vector<bool> money_paid_;                 // to see if customer in customer_in_chair_ vector has paid (match index)
   int cust_drops_;                          // # of customers dropped

   // Mutexes and condition variables to coordinate threads
   // mutex_ is used in conjuction with all conditional variables
   pthread_mutex_t mutex_;
   pthread_cond_t cond_customers_waiting_;
   vector<pthread_cond_t> cond_customer_served_;
   vector<pthread_cond_t> cond_barber_paid_;
   vector<pthread_cond_t> cond_barber_sleeping_;
  
   void init();
   string int2string(int i);
   void print(int person, string message);
};
#endif
