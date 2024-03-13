#pragma once

int CreateDomainSock();

void BindParentDomainSock(int sock_fd, char *path);

void ListenOnDomainSock(int sock_fd);
