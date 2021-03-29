/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   exports.h
 * Author: mhill
 *
 * Created on September 6, 2017, 10:32 AM
 */

#ifndef ALPRSUPPORT_EXPORTS_H
#define ALPRSUPPORT_EXPORTS_H

#ifdef _WIN32
  #define OPENALPRSUPPORT_DLL_EXPORT __declspec( dllexport )
#else
  #define OPENALPRSUPPORT_DLL_EXPORT 
#endif


#endif /* ALPRSUPPORT_EXPORTS_H */

