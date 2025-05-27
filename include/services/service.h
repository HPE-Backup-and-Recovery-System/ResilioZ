#ifndef SERVICE_H_
#define SERVICE_H_



  class Service {
    public:
     virtual void Run() = 0;
     virtual void Log() = 0;
     virtual ~Service() = default;
   };


#endif  // SERVICES_SERVICE_H_