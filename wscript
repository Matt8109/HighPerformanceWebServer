#! /usr/bin/evn python
# encoding: utf-8

import sys
import Options, UnitTest
from TaskGen import feature, after
import Task, Utils

srcdir = '.'
blddir = 'build'

def set_options(opt):
    opt.tool_options('compiler_cxx')

    opt.add_option('--debug', help='Build debug variant',
                   action='store_true', dest="build_debug", default=True
                  )

    opt.add_option('--release', help='Build release variant',
                  action='store_false', dest="build_debug"
                 )

def configure(conf):
    conf.check_tool('compiler_cxx')

    #
    # Configure platform
    #
    if sys.platform.startswith('linux'):
        conf.env.CXXDEFINES = ['LINUX']
    elif sys.platform.startswith('darwin'):
        conf.env.CXXDEFINES = ['MAC_OS']

    #
    # Configure libraries
    #
    conf.env.LIB_PTHREAD = [ 'pthread' ]
    conf.env.LIB_PROFILE = [ 'profiler' ]
    conf.env.LIB_RT = ['rt']
    conf.env.LIB_TCMALLOC = [ 'tcmalloc' ]

    #
    # Configure a debug environment
    #
    env = conf.env.copy()
    env.set_variant('debug')
    conf.set_env_name('debug', env)
    conf.setenv('debug')
    conf.env.CXXFLAGS = ['-g', '-Wall', '--pedantic',
                         '-fno-omit-frame-pointer']
    conf.env.LINKFLAGS = ['-export-dynamic']

    #
    # Configure a release environment
    #
    env = conf.env.copy()
    env.set_variant('release')
    conf.set_env_name('release', env)
    conf.setenv('release')
    conf.env.CXXFLAGS = ['-O3', '-g', '-Wall', '--pedantic',
                         '-fno-omit-frame-pointer']

def build(bld):
    #****************************************
    # libs
    #

    # header only libs; just documenting
    # lock.hpp
    # unit_test.hpp

    bld.new_task_gen( features = 'cxx cstaticlib',
                      source = """ log_message.cpp
                                   log_writer.cpp
                                   param_map.cpp
                               """,
                      includes = '.. .',
                      uselib = 'PTHREAD',
                      target = 'logging',
                      name = 'logging'
                    )

    bld.new_task_gen( features = 'cxx cstaticlib',
                      source = """ circular_buffer.cpp
                                   list_set.cpp
                               """,
                      includes = '.. .',
                      uselib = 'PTHREAD',
                      uselib_local = 'logging',
                      target = 'base',
                      name = 'base'
                    )

    bld.new_task_gen( features = 'cxx cstaticlib',
                      source = """ buffer.cpp
                                   child_process.cpp
                                   file_cache.cpp
                                   op_generator.cpp
                                   thread.cpp
                                   thread_pool_fast.cpp
                                   thread_pool_normal.cpp
                                   thread_registry.cpp
                                   signal_handler.cpp
                               """,
                      includes = '.. .',
                      uselib = 'PTHREAD',
                      uselib_local = 'logging',
                      target = 'concurrency',
                      name = 'concurrency'
                    )

    bld.new_task_gen( features = 'cxx cstaticlib',
                      source = """ acceptor.cpp
                                   connection.cpp
                                   descriptor_poller_epoll.cpp
                                   io_manager.cpp
                                   request_stats.cpp
                                   service_manager.cpp
                                   ticks_clock.cpp
                                """,
                      includes = '.. .',
                      uselib = 'PTHREAD',
                      uselib_local = 'concurrency',
                      target = 'net_server',
                      name = 'net_server'
                    )

    bld.new_task_gen( features = 'cxx cstaticlib',
                      source = """ http_connection.cpp
                                   http_parser.cpp
                                   http_request.cpp
                                   http_service.cpp
                               """,
                      includes = '.. .',
                      uselib = 'PTHREAD',
                      uselib_local = 'net_server concurrency',
                      target = 'http_server',
                      name = 'http_server'
                    )

    bld.new_task_gen( features = 'cxx cstaticlib',
                      source = """ kv_connection.cpp
                                   kv_service.cpp
                               """,
                      includes = '.. .',
                      uselib = 'PTHREAD',
                      uselib_local = 'net_server concurrency',
                      target = 'kv_server',
                      name = 'kv_server'
                    )


    #****************************************
    # tests / benchmarks
    #

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'buffer_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'buffer_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'buffer_benchmark.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'buffer_benchmark'
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'callback_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'callback_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'callback_benchmark.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'callback_benchmark'
                      )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'circular_buffer_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'base',
                      target = 'circular_buffer_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'connection_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'net_server',
                      target = 'connection_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'demo_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'logging',
                      target = 'demo_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'false_sharing_benchmark.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'false_sharing_benchmark',
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'file_cache_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'file_cache_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'hazard_pointers_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'hazard_pointers_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'http_parser_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'http_server concurrency',
                      target = 'http_parser_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'list_set_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'base',
                      target = 'list_set_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'lock_free_list_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'lock_free_list_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'markable_pointer_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'markable_pointer_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'param_map_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'logging',
                      target = 'param_map_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'request_stats_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'net_server',
                      target = 'request_stats_test',
                      unit_test = 1
                    )
    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'signal_handler_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'signal_handler_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'spinlock_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'spinlock_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'thread_local_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'thread_local_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'thread_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'thread_test',
                      unit_test = 1
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'thread_pool_benchmark.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'thread_pool_benchmark',
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'thread_pool_test.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = 'concurrency',
                      target = 'thread_pool_test',
                      unit_test = 1
                    )

    #****************************************
    # Binaries
    #

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'server.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = """ kv_server
                                         http_server
                                         net_server
                                         concurrency
                                     """,
                      target = 'server'
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'client_sync.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = """ http_server
                                         net_server
                                         concurrency
                                     """,
                      target = 'client_sync'
                    )

    bld.new_task_gen( features = 'cxx cprogram',
                      source = 'client_async.cpp',
                      includes = '.. .',
                      uselib = '',
                      uselib_local = """ http_server
                                         net_server
                                         concurrency
                                     """,
                      target = 'client_async'
                    )

    #
    # Build debug variant, if --debug was set
    #
    if Options.options.build_debug:
        clone_to = 'debug'
    else:
        clone_to = 'release'
    for obj in [] + bld.all_task_gen:
        obj.clone(clone_to)
        obj.posted = True # dont build in default environment
