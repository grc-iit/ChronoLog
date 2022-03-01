# ChronoLog

ChronoLog: A High-Performance Storage Infrastructure for Activity and Log Workloads (NSF CSSI 2104013)

## ChronoLog Project Synopsis

This project will design and implement ChronoLog, a distributed and tiered shared log storage ecosystem. ChronoLog uses physical time to distribute log entries while providing total log ordering. It also utilizes multiple storage tiers to elastically scale the log capacity (i.e., auto-tiering). ChronoLog will serve as a foundation for developing scalable new plugins, including a SQL-like query engine for log data, a streaming processor leveraging the time-based data distribution, a log-based key-value store, and a log-based TensorFlow module.

## Workloads and Applications

Modern applications spanning from Edge to High Performance Computing (HPC) systems, produce and process log data and create a plethora of workload characteristics that rely on a common storage model: **the distributed shared log**.

![Log centric paradigm](/doc/images/log_centric_paradigm.svg)

## Features

![Feature matrix](/doc/images/feature-matrix.png)

------
# Coming soon ...

For more details about the ChronoLog project, please visit our website http://www.cs.iit.edu/~scs/assets/projects/ChronoLog/ChronoLog.html.
