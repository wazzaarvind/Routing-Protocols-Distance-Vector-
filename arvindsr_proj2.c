/* 
 
ROUTING PROTOCOLS

Distance Vector Routing Protocol

Course Project 2

Subect : Modern Networking Concepts 

Description :  We implement a simplified version of the Distance Vector Protocol. 
The protocol will be run on top of servers (behaving as routers) using UDP. 
Each server runs on a machine at a pre-defined port number

Subject Code : CSE 589

University at Buffalo, the State University of New York

Created by Arvind Srinivass Ramanathan on 11/15/2016

Submitted by Arvind Srinivass Ramanathan on 11/21/2016

UBIT Name : arvindsr

Email ID : arvindsr@buffalo.edu


Person Number : 50205659 

*/ 






#include<arpa/inet.h>
#include<stdio.h>
#include<net/if.h>
#include<string.h>
#include<stdlib.h>
#include<netdb.h>
#include<ifaddrs.h>
#include<errno.h>
#include<sys/wait.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<sys/ioctl.h>
#include<unistd.h>
#include<arpa/nameser.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<limits.h>
#include<ctype.h>


// Main Server declaration with different data members

struct server
{
	uint16_t server_id;
	char server_port[200];
	char server_ip[16];
	int cost[10];
	int dest[10];
	int destcount;
	int nexthop;
	int exists; // To keep track whether a server is alive or not
	uint16_t currentserver_cost;
	int count_to_kill; // To discover that a neighbour server is dead
	int neighbour_or_not; // To keep track whether a server is a neighbour or not

};


// Global Delcarations
struct server s[5];

int no_servers=0,no_neighbours=0;
char msg1[400];
char my_port[50];
char my_ip[300];
uint16_t my_id=0;
uint16_t matrix[5][5];
int ct;
int sock_fd;
int num_of_pkts_received=0;
char response_message[500];
char sadd[INET6_ADDRSTRLEN];
uint16_t linkcosts[5];


//Implementation of Bellman Ford Algorithm


void bellman_ford()
{
	int i=0,j=0,mincost=0;

	// Loop through the servers

	for(i=0;i<no_servers;i++)
	{
		if(i!=my_id-1) // Ignore this server, we have it's routing table already!
		{
			// Assign initial mincost
			mincost=25000;
	
			for(j=0;j<no_servers;j++) // Loop paths through all nodes using the matrix at hand
			{
				if(j!=my_id-1&&s[j].neighbour_or_not==1)//&&j!=i // Ignore path taken through this server itself, stupid!
				{
					
					if(linkcosts[j]+matrix[j][i]<mincost) // If this route gives a smaller value, update it!
					{	
						mincost=linkcosts[j]+matrix[j][i];
						s[i].nexthop=s[j].server_id;
					}
				}
					
			}
			//Finally Update the values for a particular distance from server A to server B
			
			matrix[my_id-1][i]=mincost;
			s[i].currentserver_cost=mincost;
		}

	}
}





// Error Function

void error(char *message)
{
	perror(message);
	exit(1);
}


/* EXTREMELY IMPORTANT!
THIS IS THE ROUTING MESSAGE STRUCUTRE. IT IS SENT IN PACKETS OF DIFFERENT SIZES AS SPECIFIED!
*/

void construct_packet_2(void *msgtosend)
{
	
	int areatofill=8+(10*no_neighbours);
	char my[100];
	ct=0;


	struct sockaddr_in sending_sock;
	int i=0;
	char cbuff[500];
	int tip;


	struct sockaddr_in other_address;
	char temp_ip[100];

	

	// Set 0's in the memory area and use a pointer to traverse through
	memset(msgtosend,0,1000);
	void *ptr=msgtosend;


	// 2 BYTE Number of Update Fields [2 Initial Fields plus 4 for Each Server]
	// Number of Update Fields  as uint16_t
	uint16_t nouf=htons(2+(no_servers*4));
	memcpy(ptr,&nouf,sizeof(nouf));
	ptr=ptr+2;

	
	// 2 Byte Sending Server Port
	uint16_t tempport1=htons(atoi(my_port));
	memcpy(ptr,&tempport1,sizeof(tempport1));
	ptr=ptr+2;

	// 4 Byte Sending Server IP
	uint32_t bu,bu1;
	inet_pton(AF_INET,my_ip,&bu);
	//inet_ntop(AF_INET,&bu,my,INET6_ADDRSTRLEN);
	bu1=htonl(bu);
	memcpy(ptr,&bu1,sizeof(bu1));
	ptr=ptr+4;



	
	// Loop for Rest of the Servers
	for(i=0;i<no_servers;i++)
	{
		// 4 Byte Server(i) IP
		inet_pton(AF_INET,s[i].server_ip,&bu);
		bu1=htonl(bu);
		memcpy(ptr,&bu1,sizeof(bu1));
		ptr=ptr+4;

		// 2 Byte Server(i) Port
		uint16_t tempport=htons(atoi(s[i].server_port));
		memcpy(ptr,&tempport,sizeof(tempport));
		ptr=ptr+2;

		// 2 Byte Server(i) ID
		uint16_t ttid=htons(s[i].server_id);
		memcpy(ptr,&ttid,sizeof(ttid));
		ptr=ptr+2;

		// 2 Byte Cost from Sending Server to Server(i) 
		uint16_t ttcost=htons(matrix[my_id-1][i]);
		memcpy(ptr,&ttcost,sizeof(ttcost));
		ptr=ptr+2;

	}
}

void construct_packet()
{
	
	
	int i=0;
	

	//Address to send to receiver

	struct sockaddr_in other_address;

	// Loop through all servers
	while(i<no_servers)
	{
		
		// Make sure the server exists and send messages only to neighbours

		if(s[i].exists==1 && s[i].neighbour_or_not==1)
		{
			char msgtosend[1000]; // MAIN ROUTING MESSAGE TO SEND


			construct_packet_2(&msgtosend); // CONSTRUCTION OF ROUTING MESSAGE

			other_address.sin_family = AF_INET;
			


			

			inet_aton(s[i].server_ip, &other_address.sin_addr.s_addr);
			

			other_address.sin_port = htons(atoi(s[i].server_port)); 
			


			
			// Send Message

			if(sendto(sock_fd, msgtosend, sizeof(msgtosend), 0,(struct sockaddr *)&other_address,sizeof(other_address))<0) // Send Fails
			{
				
				perror("\nError while sending data!\n");

			}


			//else // Send Success , List Servers that Messages got sent to
			//{
				
				//printf("The server sent an update to the other server with ID : %d IP : %s on the port  %s\n",s[i].server_id,s[i].server_ip,s[i].server_port); 
						

			//}
		}

		i++;
	}
}




// Function to Update the Cost between Server's index 1 and index 2 with cost newcost

int updatelinks(int index1,int index2,int newcost)
{

		int k=0,i=0;

		// Update Matrix, Next Hop, Cost from Current Server according to what the cost sent is
		//25000 refers to inf
		char msg5[500];
		memset(msg5,0,500);

			matrix[index1-1][index2-1]=newcost;
			matrix[index2-1][index1-1]=newcost;
			s[index2-1].nexthop=index1;
			s[index2-1].currentserver_cost=newcost;
			linkcosts[index2-1]=newcost;

			if(newcost==25000)
			{
				s[index2-1].nexthop=-1;
				for(k=0;k<no_servers;k++)
				{
					if(s[k].nexthop==index2)
					{
						s[k].currentserver_cost=25000;
						s[k].nexthop=-1;
						matrix[index1-1][k]=25000;
						matrix[k][index1-1]=25000;

					}
				}
			}

			void *ptr1=msg5;
			struct sockaddr_in other_address;
			char word1[50]="",word2[50]="";


			other_address.sin_family = AF_INET;

			uint16_t sendk=6;
			uint16_t nouf=htons(sendk);
			memcpy(ptr1,&nouf,sizeof(nouf));
			ptr1=ptr1+2;

			// 2 Byte Server(i) ID
			uint16_t ttid=htons(index1);
			memcpy(ptr1,&ttid,sizeof(ttid));
			ptr1=ptr1+2;

			// 2 Byte Cost from Sending Server to Server(i) 
			uint16_t ttcost=htons(newcost);
			memcpy(ptr1,&ttcost,sizeof(ttcost));
			ptr1=ptr1+2;

			

			inet_aton(s[index2-1].server_ip, &other_address.sin_addr.s_addr);
			

			other_address.sin_port = htons(atoi(s[index2-1].server_port)); 
			


			
			// Send Message

			if(sendto(sock_fd, msg5, sizeof(msg5), 0,(struct sockaddr *)&other_address,sizeof(other_address))<0) // Send Fails
			{
				
				perror("\nError while sending data!\n");

			}

		//construct_packet(); // Send Updated values to neighbours

		return 1;

}


//Main Function

int main(int argc,char *argv[])
{
	int k=0,i=0,j=0;
	fd_set fds,readfds,kfds;
	socklen_t sin_size;
	int len=0;
	char *ip_version;
	int x,y,z;
	int fdmax,check_read,max_fd,addrlen;
	struct sockaddr_in *addr_v4,sa,si_other,my_ip_struct;
	struct sockaddr_storage cl_addr;
	char host_n[50];
	struct timeval tv;
	void *addr;
	char filename[30],ri[30];
	int routing_interval;
	struct addrinfo hints,*info_server,*p;
	char *token;

	// Read Topology File Name and Routing Interval from Commmand Line arguments
	strcpy(filename,argv[2]);
	strcpy(ri,argv[4]);
	routing_interval=atoi(ri);


	char line1[100],line2[100];
	FILE *f1;

	// Open File and get Number of Servers and Number of Neighbours
	f1=fopen(filename,"r");
	fgets(line1,90,f1);
	no_servers=atoi(line1);
	fgets(line2,90,f1);
	no_neighbours=atoi(line2);


	// Read ID,IP,Port for each Server and Set other initial values
	for(k=0;k<no_servers;k++)
	{

		fgets(line1,90,f1);
		token=strtok(line1," ");
		char servid[20];
		char servip[100];
		strcpy(servid,token);
		s[k].server_id=atoi(servid);
		token=strtok(NULL," ");
		strcpy(s[k].server_ip,token);
		token=strtok(NULL,"\n");
		strcpy(s[k].server_port,token);
		s[k].exists=1;
		s[k].nexthop=-1;
		s[k].neighbour_or_not=0;
		s[k].currentserver_cost=25000;
		s[k].count_to_kill=0;


	}


	// Set initial values 
	for(i=0;i<no_servers;i++)
	{
		linkcosts[i]=25000;
		for(j=0;j<no_servers;j++)
		{
			if(i!=j)
			{
				s[i].cost[j]=25000;
				matrix[i][j]=25000;

			}
			else
			{
				s[i].cost[j]=0;
				matrix[i][j]=0;
			}
		}
	}
	
	int m;


	memset(&hints, 0, sizeof hints);
  

  	hints.ai_family=AF_UNSPEC;
  	hints.ai_socktype=SOCK_DGRAM;
  	hints.ai_flags=AI_PASSIVE; 



   	int rev=0;
   	gethostname(host_n,sizeof host_n);




   	//Getting Address information of the given host

  	rev=getaddrinfo(host_n,NULL,&hints,&info_server);

    if(rev!=0)
   	{
    
    	fprintf(stderr,"getaddrinfo:%s\n", gai_strerror(rev));
    
    	//return 1;
   	}

   	p=info_server;

  	
  	while(p!=NULL)
  	{
   		
   		if(p->ai_family==AF_INET)
   		{

    		struct sockaddr_in *ipv4=(struct sockaddr_in *)p->ai_addr;
    		addr = &(ipv4->sin_addr);
    		ip_version = "IPv4";

   		
   		}
   		
   		inet_ntop(p->ai_family,addr,sadd,sizeof sadd); // sadd has the IP Address of the Server we are running on
   		p=p->ai_next;

   	}





  	for(i=0;i<no_servers;i++)
  	{
  		// Find out which entry in the Structure Server matches the Server we are running on by comparing with IP and store, we use it throughout!

  		if(strcmp(inet_ntop(info_server->ai_family,addr,sadd, sizeof sadd),s[i].server_ip)==0)
  		{
  			my_id=s[i].server_id;
  			strcpy(my_ip,s[i].server_ip);
  			strcpy(my_port,s[i].server_port);
  			linkcosts[my_id-1]=0;
  			
  		}
  	}
   
    
	
		// Read the Costs from the next few files of the topology file and store them

		for(k=0;k<no_neighbours;k++)
		{


			char m1[20],temp1[20],temp2[20];
			int temp,tcost;
			fgets(line1,90,f1);
			token=strtok(line1," ");
			strcpy(m1,token);
			m=atoi(m1);
			token=strtok(NULL," ");
			strcpy(temp1,token);
			temp=atoi(temp1);
			token=strtok(NULL,"\n");
			strcpy(temp1,token);
			tcost=atoi(temp1);
			s[m-1].cost[temp-1]=tcost;
			matrix[m-1][temp-1]=tcost;
			s[temp-1].cost[m-1]=tcost;
			matrix[temp-1][m-1]=tcost;
			s[temp-1].neighbour_or_not=1;
			//check
			s[temp-1].nexthop=my_id;
			s[temp-1].currentserver_cost=tcost;
			linkcosts[temp-1]=tcost;


			
		}


		s[my_id-1].currentserver_cost=0;
		s[my_id-1].nexthop=my_id;
		

		



	// MAKE A CALL FOR FINDING OWN IP ADDRESS IF REQUIRED





	// Set timeout value to the routing interval which is the command line argument taken

	tv.tv_sec=routing_interval;
	tv.tv_usec=0;



	// Create a Socket ( Main Server Socket )

	if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{ 
		perror("socket");
		return -1; 
    }


	memset(&my_ip_struct, 0 , sizeof(my_ip_struct));
	my_ip_struct.sin_family = AF_INET; 
    my_ip_struct.sin_addr.s_addr = INADDR_ANY; 
    my_ip_struct.sin_port = htons(atoi(my_port)); 

    //Refreshing
    if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&(int){ 1 },sizeof(int))==-1)
  	{
   		error("\nError while setting socketopt\n");
   		exit(1);
  	}


   
  	//Binding the Socket

    if (bind(sock_fd, (struct sockaddr*)&my_ip_struct, sizeof(my_ip_struct)) < 0)
    { 
		perror("bind"); 
		return -1; 
    }
	


	

	//Setting all File Descriptors to 0
	FD_ZERO(&fds);
	FD_ZERO(&readfds);	

	// Setting the Master Socket to main socket and 0 for Keyboard input

	FD_SET(sock_fd,&fds);
	FD_SET(0,&fds);
	max_fd=sock_fd;

	//Keep Looping

	for(;;)
	{	
		
		readfds=fds;
		
		check_read=select(max_fd+1,&readfds,NULL,NULL,&tv);
		
		//Error
		if(check_read==-1)
		{
			//check
			perror("\nSelect causes an error!\n");
			continue;
		}

		// Nothing set, feel free to send messages

		else if(check_read==0) //Send message to all neighbours
		{

			//Keep count if a Server is still discoverable
			for(i=0;i<no_servers;i++)
				if(s[i].neighbour_or_not==1)
					s[i].count_to_kill++; 

			for (i = 0; i < no_servers; i++)
			{ 

				if(s[i].neighbour_or_not==1)
				{
					// Kill the Neighbour Server if count is 3(3 update intervals)
					if (s[i].count_to_kill == 3)
					{

						s[i].neighbour_or_not=0;
						s[i].nexthop=-1;
						s[i].exists = 0;
						s[i].currentserver_cost=25000;
						linkcosts[i]=25000;
						
						matrix[my_id-1][s[i].server_id-1]= 25000;
						matrix[s[i].server_id-1][my_id-1]= 25000; 
						for(j=0;j<no_servers;j++)
						{
							if(s[j].nexthop==s[i].server_id)
							{
								s[j].currentserver_cost=25000;
								s[j].nexthop=-1;
								
							}
						}

					}
				}
			}

			//To Construct the Packet and send messages
			construct_packet();

			//Reset routing interval, messages were sent
			tv.tv_sec=routing_interval;
			continue;


		}


		//Loop through all file descriptors

		for(i = 0; i <= max_fd; i++) 
		{	

			 if (FD_ISSET(i, &readfds)) 
            	{ 
            		
            		if(i==sock_fd) //Neighbours' messages are here!

                	 	{
                	 		char my[199],my1[199],temprecvmsg[2000];
                	 		uint16_t arrayrecv[100],arrayrecv1[100];
                	 		char recvmsg[2000];
                	 		void *ptrrecv;
                	 		
                	 		// Receive the message
                	 		if(recvfrom(sock_fd,&recvmsg,sizeof(recvmsg),0,(struct sockaddr *)&cl_addr,&sin_size)<0)
                	 			perror("\nError while receiving data!\n");
                	 		
                	 		


                	 		else
                	 		{
                	 			strcpy(temprecvmsg,recvmsg);
                	 			uint16_t tempcosty,ty1,tempidy,tid1,tc,tc1;
                	 			void *pkt1=recvmsg;
                	 			memcpy(&tc,pkt1,2);
	                	 		tc1=ntohs(tc);
	                	 		pkt1=pkt1+2;
                	 			if(tc1==6)
                	 			{
                	 				printf("\nUpdate occurred!\n");
                	 				memcpy(&tempidy,pkt1,2);
	                	 			tid1=ntohs(tempidy);
	                	 			pkt1=pkt1+2;
                	 				memcpy(&tempcosty,pkt1,2);
                	 				ty1=ntohs(tempcosty);
                	 				pkt1=pkt1+2;
                	 				if(s[tid1-1].neighbour_or_not==0)
                	 					continue;
                	 				if(s[tid1-1].currentserver_cost>ty1)
                	 					{
                	 						s[tid1-1].currentserver_cost=ty1;
                	 						s[tid1-1].nexthop=tid1;
                	 					}
                	 				linkcosts[tid1-1]=ty1;
                	 				matrix[my_id-1][tid1-1]=ty1;
                	 				matrix[tid1-1][my_id-1]=ty1;
                	 				continue;
                	 			}



                	 			int p=0;

                	 			//Pointer to starting of message
                	 			void *pkt=recvmsg;


                	 			uint16_t idtotrack=0;	// ID To Track Sending ID
                	 			uint16_t tsending_port=9,tsender_fields=0,tcount=0,tsending_id=0,tsending_cost=0;
                	 			uint16_t sending_port=0,sender_fields=0,count;
                	 			char sender_ip[100];

                	 			// Receive 2 Byte Number of Updated Fields
                	 			memcpy(&tsender_fields,pkt,2);
                	 			sender_fields=ntohs(tsender_fields);
                	 			count=(sender_fields-2)/4;
                	 			pkt=pkt+2;

                	 			uint16_t serverlistport[count],serverlistid[count],serverlistcost[count];
                	 			uint32_t fakesending_ip,tsending_ip;
                	 			char serverlistip[count][100];

                	 			for(p=0;p<count;p++)
                	 			{
                	 				serverlistport[p]=0;
                	 				serverlistid[p]=0;
                	 				serverlistcost[p]=0;
                	 			}

                	 			

                	 			// Receive 2 Byte Sending Port
                	 			memcpy(&tsending_port,pkt,2);
                	 			sending_port=ntohs(tsending_port);
                	 			pkt=pkt+2;
                	 			
                	 			// Receivbe 4 Byte Sending IP and convert it to a String(Character Array)
                	 			memcpy(&tsending_ip,pkt,4);
                	 			fakesending_ip=ntohl(tsending_ip);
                	 			inet_ntop(AF_INET,&fakesending_ip,sender_ip,INET6_ADDRSTRLEN);
                	 			pkt=pkt+4;

                	 			//Loop through packets received for each Server
                	 			for(p=0;p<count;p++)
                	 			{

                	 				// IP Address of Server(p) in 4 Bytes
	                	 			memcpy(&tsending_ip,pkt,4);
	                	 			fakesending_ip=ntohl(tsending_ip);
	                	 			inet_ntop(AF_INET,&fakesending_ip,serverlistip[p],INET6_ADDRSTRLEN);
	                	 			pkt=pkt+4;

	                	 			// Port of Server(p) in 2 Bytes
	                	 			memcpy(&tsending_port,pkt,2);
	                	 			serverlistport[p]=ntohs(tsending_port);
	                	 			pkt=pkt+2;

	                	 			// ID of Server(p) in 2 Bytes
	                	 			memcpy(&tsending_id,pkt,2);
	                	 			serverlistid[p]=ntohs(tsending_id);
	                	 			pkt=pkt+2;

	                	 			// Cost to Server(p) from Current Server in 2 Bytes
	                	 			memcpy(&tsending_cost,pkt,2);
	                	 			serverlistcost[p]=ntohs(tsending_cost);
	                	 			pkt=pkt+2;
	                	 		}

	                	 		int flag=-1;

	                	 		for(p=0;p<no_servers;p++)
	                	 		{
	                	 			if(strcmp(sender_ip,s[p].server_ip)==0) // Check if you received a message from a Valid Server, else ignore message!
	                	 				{
	                	 					idtotrack=s[p].server_id;
	                	 					if(s[idtotrack-1].exists==1&&s[idtotrack-1].neighbour_or_not==1)
	                	 					flag=1;
	                	 					break;
	                	 				}

	                	 		}

	                	 		if(flag==-1) // Ignore message from Invalid Server
	                	 			{

	                	 				continue;
	                	 			}
	                	 		
	                	 		//Update Matrix Values and Costs
	                	 		for(p=0;p<count;p++)
	                	 		{	
	                	 			
	                	 			matrix[idtotrack-1][serverlistid[p]-1]=serverlistcost[p];
	                	 		}
	                	 		
	                	 		//check
	                	 		//matrix[my_id-1][idtotrack-1]=matrix[idtotrack-1][my_id-1];
	                	 		//s[idtotrack-1].currentserver_cost=matrix[my_id-1][idtotrack-1];


	                	 	

                	 			printf("\nRECEIVED A MESSAGE FROM SERVER %d\n",idtotrack);
                	 			

                	 			// Call Bellman Ford to Compute Least Distances
								bellman_ford();

								//Increment Packets
								num_of_pkts_received++;
								

								//You received a message, set back count to kill to 0
								s[idtotrack-1].count_to_kill=0;
	

                	 		}

                	 	}

                	 	// Keyboard Input
	                    else if(i==0)

	                    {

	                    	char buff[50]="";
							int x1=0,y1=0,z1=0,x2=0;
	                    	int v=0,u=0;
	                    	char ipp[50];
	                    	fgets(buff,sizeof(buff),stdin);
							len = strlen(buff) - 1;
				     		if (buff[len] == '\n')
				            buff[len] = '\0';

				        	//Convert Input String to Upper Case
				        	for(v=0;buff[v]!=' ';v++)
				        	{
				        	
				        	buff[v]=toupper(buff[v]);
				        	

				        	}

				        	//To Detect Number of Parameters
				        	for(j=0;j<strlen(buff);j++)
				        	{
				        		if(buff[j]==' ')
				        			u++;

				        	}

				        	//One parameter, so for DISABLE
					        if(u==1)
					        {

					        			token=strtok(buff," ");
					        			strcpy(buff,token);
					        			token=strtok(NULL," ");
					        			strcpy(ipp,token);
					        			x2=atoi(ipp);
					        			

					        }


					        // Three Parameters , so for UPDATE
					        else if(u==3)
			        		{

			        			char rip1[100],rport1[100],rport2[100];
			        			token=strtok(buff," ");
			        			strcpy(buff,token);
			        			token=strtok(NULL," ");
			        			strcpy(rip1,token);	
			        			x1=atoi(rip1);	
			        			token=strtok(NULL," ");
			        			strcpy(rport1,token);
			        			y1=atoi(rport1);
			        			token=strtok(NULL," ");
			        			strcpy(rport2,token);

			        			//If Cost is infinity, set it to a high value like 250000
			        			if(strcasecmp(rport2,"inf")==0)
			        				z1=25000;
			        			else
			        				z1=atoi(rport2);

			        			

			        		}

			        		// For UPDATE, call Update Function
				        	if(strcmp("UPDATE",buff)==0)
				         	{	
				         		if(u!=3)
				         		{
				         			printf("\nUPDATE Please enter 3 parameters!\n");
				         			continue;
				         		}
				         		
				         		else
				         		{
				         			int flag1=-1,flag2=-1;

									for(i=0;i<no_servers;i++)
									{
										if(s[i].server_id==x1)
											{
												flag1=1;
												break;
											}
									}


									// Parameter 1 not in Server list
									if(flag1==-1)
									{
										printf("\nUPDATE Paraneter 1 is incorrect!\n");
										//return -1;
									}

									for(i=0;i<no_servers;i++)
									{
										if(s[i].server_id==y1)
											{
												flag2=1;
												break;
											}
									}

									// Parameter 2 not in Server list
									if(flag2==-1)
									{
										printf("\nUPDATE Paraneter 2 is incorrect!\n");
										//return -1;
									}


									//No UPDATE if either parameter is not in Update List
									if(flag1==-1||flag2==-1)
									{
										printf("\nUPDATE Please re-enter parameters with update command!\n");
										continue;
									}


									else if(x1==y1)
									{	
										printf("\nUPDATE The two nodes cannot be the same!\n");
										continue;
									}

									else if(s[y1-1].neighbour_or_not==0)
									{
										printf("\nUPDATE The two parameters are not neighbours!\n");
										continue;
									}

									else if(x1!=my_id)
									{
										printf("\nUPDATE Enter changes with respect to current server!\n");
										continue;
									}

									else
										updatelinks(x1,y1,z1);


				         		}
				         		printf("\n%s SUCCESS\n",buff);
				         		continue;
				         	}



				         	// For STEP, construct the packet and send messages to all neighbours
				         	else if(strcmp("STEP",buff)==0)
         					{
         						construct_packet();
         						printf("\n%s SUCCESS\n",buff);
         						continue;
         					}


         					// For PACKETS, print the current number of Packets and reset it to 0
         					else if(strcmp("PACKETS",buff)==0)
         					{
         						printf("\nThe number of packets received after last time is : %d\n",num_of_pkts_received);
								num_of_pkts_received=0;
								printf("\n%s SUCCESS\n",buff);	
         						continue;
         					}

         					// For DISABLE, disable the Server entered as Parameter, it is not a neighbour anymore
							else if(strcmp("DISABLE",buff)==0)
         					{
         						if(u==1)
         						{	
         							int flag3=-1;

         							for(i=0;i<no_servers;i++)
									{
										if(s[i].server_id==x2)
											{
												flag3=1;
												break;
											}
									}
									if(flag3==-1)
									{
										printf("\nDISABLE Paraneter is incorrect!\n");
										continue;
									}
									if(s[x2-1].neighbour_or_not!=1)
									{
										printf("\nYou cannot disable non neighbours!\n");
										continue;
									}
	         						s[x2-1].neighbour_or_not=0;
									s[x2-1].currentserver_cost=25000;
									linkcosts[x2-1]=25000;
									s[x2-1].nexthop=-1;
									
									matrix[my_id-1][x2-1]=25000;
									matrix[x2-1][my_id-1]=25000;




									printf("\n%s SUCCESS\n",buff);	
	         						continue;
	         					}
	         					else
	         						printf("\nDISABLE Please Enter one Server ID as Parameter!\n");
         					}

         					// For DISPLAY, display the current Routiing Table
         					else if(strcmp("DISPLAY",buff)==0)
         					{
         						
         						printf("\nServer ID\t Next Hop\t Cost\n");
								for (i = 0; i < no_servers; i++)
								{	
									if(s[i].currentserver_cost==25000)
										printf ("%d\t\t %d\t\t INF\n",s[i].server_id,s[i].nexthop);
									else 	
										printf ("%d\t\t %d\t\t %d\n",s[i].server_id,s[i].nexthop,s[i].currentserver_cost); 
								}
								printf("\n%s SUCCESS\n",buff);	
								continue;

							}
         						
       
							// For CRASH, kill a server by Closing the Socket
							else if(strcmp("CRASH",buff)==0)
         					{
         						close(sock_fd);
         						printf("\n%s SUCCESS\n",buff);	
								return 1;
								continue;
         						
         					}

						}
					}
				}
			}

		// Close the Socket
		close(sock_fd);
		printf("Closed socket bound to port %s \n",my_port);

	return 0;
}

// THE END




