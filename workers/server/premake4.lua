solution "Server Worker"
	configurations { "debug", "release" }

project "server"
	kind "ConsoleApp"
	language "C++"
	files { "src/**" }
	links { "WorkerSdk", "CoreSdk", "protobuf", "gpr", "grpc", "grpc++", "z", "RakNetLibStatic", "ssl" }
    libdirs { "worker_sdk/lib" }
    includedirs { "worker_sdk/include" }
	objdir "build/obj"
	buildoptions { "-std=c++14" }

	configuration "debug"
		flags { "Symbols", "ExtraWarnings" }

	configuration "release"
		flags { "Optimize" }
