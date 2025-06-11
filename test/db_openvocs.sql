--
-- PostgreSQL database dump
--

-- Dumped from database version 16.3 (Debian 16.3-1.pgdg120+1)
-- Dumped by pg_dump version 16.2 (Debian 16.2-1)

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: events; Type: TABLE; Schema: public; Owner: openvocs
--

CREATE TABLE public.events (
    usr character varying(300),
    role character varying(300),
    loop character varying(200),
    evstate character varying(15),
    evtime timestamp without time zone
);


ALTER TABLE public.events OWNER TO openvocs;

--
-- Name: recordings; Type: TABLE; Schema: public; Owner: openvocs
--

CREATE TABLE public.recordings (
    id character(36),
    uri character varying(300),
    loop character varying(200),
    starttime timestamp without time zone,
    endtime timestamp without time zone
);


ALTER TABLE public.recordings OWNER TO openvocs;

--
-- Data for Name: events; Type: TABLE DATA; Schema: public; Owner: openvocs
--

COPY public.events (usr, role, loop, evstate, evtime) FROM stdin;
anton	taucher	strippe	RECV	2024-02-12 12:00:00
anton	taucher	strippe	RECV_OFF	2024-02-12 12:00:03
bertram	schwimmer	strippe	SEND	2024-02-12 11:00:01
caesar	schwimmer	strippe	SEND	2024-02-12 11:30:10
dora	schwimmer	strippe2	SEND	2024-02-12 11:30:10
dora	schwimmer	strippe2	RECV	2024-02-12 11:30:10
dora	schwimmer	strippe2	RECV_OFF	2024-02-12 11:35:15
dora	schwimmer	strippe2	SEND_OFF	2024-02-12 11:31:15
\.


--
-- Data for Name: recordings; Type: TABLE DATA; Schema: public; Owner: openvocs
--

COPY public.recordings (id, uri, loop, starttime, endtime) FROM stdin;
1111111111-2222222222-3333          	uri://111111111-2222222222-3333	strippe2	2024-02-12 11:29:15	2024-02-12 11:32:00
1111111111-2222222222-3334          	uri://111111111-2222222222-3334	strippe	2024-02-12 11:28:15	2024-02-12 11:29:00
\.


--
-- PostgreSQL database dump complete
--

