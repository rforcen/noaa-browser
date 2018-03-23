# noaa-browser
browser for noaa daily observation 'all' file : ghcnd_all.tar.gz, supports index, near-sql search and multidimensional projections and graph, intensive cpu operations implemented with multithreads, generates a 64 bit hash for main indexes (country, station, element, year, month) and a statistic for month observation
developed w/ Qt 5.10 on osx, should port to win/linux almost directly
tar.gz management using libarchieve
