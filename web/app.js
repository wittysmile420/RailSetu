(() => {
  "use strict";

  const STORE_KEY = "railsetu.web.v1";
  const PRIORITY = { SENIOR: 3, LADIES: 2, TATKAL: 1, GENERAL: 0 };
  const CATEGORY_COLORS = {
    SENIOR: "#cda4ff", LADIES: "#70d9ee", TATKAL: "#f0bf72", GENERAL: "#9db1bd"
  };
  const trainSeed = [
    { id: "RS101", number: "12951", name: "Rajdhani Express", from: "Mumbai", fromCode: "BOM", to: "New Delhi", toCode: "NDL", depart: "16:55", arrive: "08:35", fare: 2450, coaches: [{ id: "A1", type: "AC", seats: 24 }, { id: "S1", type: "Sleeper", seats: 16 }] },
    { id: "RS205", number: "12123", name: "Deccan Queen", from: "Mumbai", fromCode: "BOM", to: "Pune", toCode: "PUN", depart: "07:10", arrive: "10:25", fare: 850, coaches: [{ id: "C1", type: "AC Chair", seats: 20 }, { id: "D1", type: "General", seats: 12 }] },
    { id: "RS330", number: "12007", name: "Shatabdi Express", from: "Chennai", fromCode: "MAA", to: "Bengaluru", toCode: "BLR", depart: "06:00", arrive: "10:55", fare: 1280, coaches: [{ id: "C1", type: "AC Chair", seats: 14 }, { id: "D1", type: "General", seats: 10 }] }
  ];
  const names = ["Aarav Mehta", "Isha Kapoor", "Rohan Desai", "Mira Nair", "Kabir Shah", "Anaya Rao", "Dev Malhotra", "Tara Iyer", "Arjun Bose", "Diya Khanna", "Neil Thomas", "Sara Menon", "Vihaan Kulkarni", "Myra Joshi", "Reyansh Sethi", "Kiara Pillai", "Ayaan Verma", "Zoya Fernandes", "Advik Banerjee", "Avni Reddy"];
  const activityLabels = {
    BOOKING: ["Reservation confirmed", "New passenger journey created", "Seat allocation completed"],
    WAITLIST: ["Passenger added to priority queue", "FIFO position assigned", "Waitlist entry recorded"],
    CANCEL: ["Cancellation processed", "Refund calculated", "Seat released to the network"],
    PROMOTE: ["Waitlist passenger promoted", "Seat reassigned from FIFO queue"],
    SYSTEM: ["Local data snapshot saved", "Synchronization check completed", "System health nominal"]
  };

  const $ = (selector, root = document) => root.querySelector(selector);
  const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];
  const escapeHtml = (value) => String(value).replace(/[&<>"']/g, (char) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  })[char]);
  const timestamp = () => new Date().toISOString();
  const formatMoney = (value) => new Intl.NumberFormat("en-IN", {
    style: "currency", currency: "INR", maximumFractionDigits: 0
  }).format(value);
  const shortTime = (iso) => {
    const date = new Date(iso);
    if (Number.isNaN(date.getTime())) return "just now";
    const seconds = Math.max(0, Math.floor((Date.now() - date.getTime()) / 1000));
    if (seconds < 45) return `${seconds || 1}s ago`;
    if (seconds < 3600) return `${Math.floor(seconds / 60)}m ago`;
    if (seconds < 86400) return `${Math.floor(seconds / 3600)}h ago`;
    return date.toLocaleDateString();
  };

  function makeSeed() {
    const state = { trains: structuredClone(trainSeed), bookings: [], events: [], sequence: 7001 };
    const perTrain = [29, 22, 16];
    trainSeed.forEach((train, trainIndex) => {
      const capacity = train.coaches.reduce((sum, coach) => sum + coach.seats, 0);
      for (let i = 0; i < perTrain[trainIndex]; i += 1) {
        const seatIndex = i;
        const coach = train.coaches.find((item, index) => {
          const previous = train.coaches.slice(0, index).reduce((sum, c) => sum + c.seats, 0);
          return seatIndex < previous + item.seats;
        }) || train.coaches[0];
        const before = train.coaches.slice(0, train.coaches.indexOf(coach)).reduce((sum, item) => sum + item.seats, 0);
        const category = i % 13 === 0 ? "SENIOR" : i % 8 === 0 ? "LADIES" : i % 6 === 0 ? "TATKAL" : "GENERAL";
        const name = names[(i + trainIndex * 5) % names.length];
        const sequence = state.sequence++;
        state.bookings.push({
          pnr: `RS${String(sequence).padStart(8, "0")}`, sequence, trainId: train.id,
          passenger: name, category, seats: 1, coachId: coach.id,
          seatLabels: [`${coach.id}-${String(seatIndex - before + 1).padStart(2, "0")}`],
          status: "CONFIRMED", fare: train.fare, refund: 0,
          createdAt: new Date(Date.now() - (i + trainIndex * 4) * 95000).toISOString(),
          cancelledAt: null
        });
      }
      const overflow = capacity === 0 ? 0 : (trainIndex === 0 ? 3 : trainIndex === 1 ? 2 : 1);
      for (let i = 0; i < overflow; i += 1) {
        const sequence = state.sequence++;
        const category = ["GENERAL", "SENIOR", "TATKAL"][i % 3];
        state.bookings.push({
          pnr: `RS${String(sequence).padStart(8, "0")}`, sequence, trainId: train.id,
          passenger: names[(i + 11 + trainIndex) % names.length], category,
          seats: 1, coachId: "", seatLabels: [], status: "WAITLISTED",
          fare: train.fare, refund: 0,
          createdAt: new Date(Date.now() - (i + 1) * 57000).toISOString(), cancelledAt: null
        });
      }
    });
    state.events = [
      { type: "SYSTEM", message: "RailSetu control center initialized", at: new Date(Date.now() - 3 * 60000).toISOString(), detail: "Local reservation state ready" },
      { type: "PROMOTE", message: "FIFO queue reconciliation completed", at: new Date(Date.now() - 11 * 60000).toISOString(), detail: "Priority tiers checked" },
      { type: "BOOKING", message: "Rajdhani Express departure manifest refreshed", at: new Date(Date.now() - 23 * 60000).toISOString(), detail: "Train RS101" },
      { type: "SYSTEM", message: "Atomic local snapshot saved", at: new Date(Date.now() - 34 * 60000).toISOString(), detail: "Browser storage" }
    ];
    return state;
  }

  function loadState() {
    try {
      const raw = localStorage.getItem(STORE_KEY);
      if (!raw) return makeSeed();
      const parsed = JSON.parse(raw);
      if (!Array.isArray(parsed.trains) || !Array.isArray(parsed.bookings) || !Array.isArray(parsed.events)) return makeSeed();
      return parsed;
    } catch (error) {
      console.warn("RailSetu browser state could not be loaded; seeded a fresh demo.", error);
      return makeSeed();
    }
  }

  let state = loadState();
  let selectedView = "overview";
  let activeFilter = "ALL";
  let currentRange = "7D";
  let simulationRunning = false;
  let preferredSeat = null;
  const saveState = () => {
    try {
      localStorage.setItem(STORE_KEY, JSON.stringify(state));
      updateSyncLabel();
    } catch (error) {
      toast("Browser storage is unavailable; this session will not persist.", "error");
      console.error("Could not save RailSetu demo state.", error);
    }
  };
  const trainFor = (id) => state.trains.find((train) => train.id === id);
  const allCapacity = (train) => train.coaches.reduce((sum, coach) => sum + Number(coach.seats), 0);
  const liveBookings = (trainId, status = "CONFIRMED") => state.bookings.filter((booking) => booking.trainId === trainId && booking.status === status);
  const usedSeatLabels = (trainId) => new Set(liveBookings(trainId).flatMap((booking) => booking.seatLabels));
  const freeSeatLabels = (train) => {
    const used = usedSeatLabels(train.id);
    return train.coaches.flatMap((coach) => Array.from({ length: Number(coach.seats) }, (_, index) =>
      `${coach.id}-${String(index + 1).padStart(2, "0")}`)).filter((label) => !used.has(label));
  };
  const confirmedCount = (trainId) => liveBookings(trainId).reduce((sum, booking) => sum + booking.seats, 0);
  const waitlistedCount = (trainId) => liveBookings(trainId, "WAITLISTED").length;
  const sortWaiting = (items) => [...items].sort((a, b) =>
    (PRIORITY[b.category] - PRIORITY[a.category]) || (a.sequence - b.sequence));
  const latestBookings = () => [...state.bookings].sort((a, b) => b.sequence - a.sequence);
  const updateSeatPickerLabel = () => {
    $("#seat-picker").textContent = preferredSeat ? `Selected ${preferredSeat.label}` : "Choose on seat map";
  };

  function addEvent(type, message, detail = "") {
    state.events.unshift({ type, message, detail, at: timestamp() });
    state.events = state.events.slice(0, 100);
    saveState();
  }

  function toast(message, kind = "success") {
    const item = document.createElement("div");
    item.className = `toast ${kind === "error" ? "error" : ""}`;
    item.textContent = message;
    $("#toast-stack").append(item);
    window.setTimeout(() => {
      item.classList.add("out");
      window.setTimeout(() => item.remove(), 220);
    }, 3200);
  }

  function askConfirm(title, message, confirmText = "Confirm") {
    return new Promise((resolve) => {
      const dialog = $("#confirm-dialog");
      $("#dialog-title").textContent = title;
      $("#dialog-message").textContent = message;
      $("#dialog-confirm").textContent = confirmText;
      dialog.returnValue = "cancel";
      dialog.addEventListener("close", () => resolve(dialog.returnValue === "confirm"), { once: true });
      dialog.showModal();
    });
  }

  function setView(view) {
    if (!$("#view-" + view)) return;
    selectedView = view;
    $$(".view").forEach((section) => section.classList.toggle("active", section.id === `view-${view}`));
    $$(".nav-item[data-view]").forEach((button) => button.classList.toggle("active", button.dataset.view === view));
    const labels = { overview: "Overview", reservations: "Reservations", bookings: "Bookings", trains: "Train network", concurrency: "Concurrency lab", analytics: "Analytics", activity: "Activity log" };
    $("#breadcrumb-current").textContent = labels[view] || "Overview";
    window.scrollTo({ top: 0, behavior: "smooth" });
    if (view === "overview") renderOverview();
    if (view === "reservations") renderReservations();
    if (view === "bookings") renderBookings();
    if (view === "trains") renderTrains();
    if (view === "analytics") renderAnalytics();
    if (view === "activity") renderActivity();
  }

  function updateSyncLabel() {
    const label = $(".sync-card>small");
    if (label) label.textContent = `Local demo · synced ${new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" })}`;
  }

  function metricCard({ label, value, icon, change, note, spark = "2,24 13,19 24,21 35,10 46,16 58,7 69,12 80,4" }) {
    return `<article class="metric-card"><div class="metric-top"><span>${escapeHtml(label)}</span><i class="metric-icon">${icon}</i></div><div class="metric-value">${escapeHtml(value)}</div><div class="metric-bottom"><span class="${change.startsWith("-") ? "" : "up"}">${escapeHtml(change)}</span><span>${escapeHtml(note)}</span></div><svg class="metric-spark" viewBox="0 0 84 34" aria-hidden="true"><path d="M${spark}"/></svg></article>`;
  }

  function countMetrics() {
    const confirmed = state.bookings.filter((item) => item.status === "CONFIRMED").length;
    const waiting = state.bookings.filter((item) => item.status === "WAITLISTED").length;
    const capacity = state.trains.reduce((sum, train) => sum + allCapacity(train), 0);
    const utilization = capacity ? Math.round(confirmed / capacity * 100) : 0;
    return { confirmed, waiting, capacity, utilization, trains: state.trains.length, total: state.bookings.length };
  }

  function renderMetrics(target = "#metric-grid") {
    const metrics = countMetrics();
    $(target).innerHTML = [
      metricCard({ label: "Confirmed passengers", value: metrics.confirmed.toLocaleString(), icon: "↗", change: "+8.4%", note: "vs. last week" }),
      metricCard({ label: "Active train routes", value: String(metrics.trains).padStart(2, "0"), icon: "⇄", change: "+2 routes", note: "network expanded", spark: "2,25 13,18 24,23 35,13 46,17 58,8 69,13 80,5" }),
      metricCard({ label: "Waitlist queue", value: metrics.waiting.toLocaleString(), icon: "◷", change: metrics.waiting ? `${metrics.waiting} pending` : "All clear", note: "priority managed", spark: "2,17 13,18 24,13 35,20 46,10 58,16 69,6 80,11" }),
      metricCard({ label: "Seat utilization", value: `${metrics.utilization}%`, icon: "◉", change: `${metrics.capacity} seats`, note: "network capacity", spark: "2,25 13,21 24,16 35,20 46,10 58,14 69,9 80,3" })
    ].join("");
  }

  function chartSvg(range = "7D") {
    const vals = range === "7D" ? [35, 44, 38, 63, 54, 72, 81, 69, 86, 73, 91, 83, 98] :
      range === "30D" ? [30, 38, 34, 52, 48, 62, 57, 76, 65, 87, 77, 95, 90] :
        [24, 35, 31, 44, 42, 56, 49, 66, 61, 75, 68, 87, 96];
    const previous = vals.map((value, i) => Math.max(8, value - 11 + (i % 3) * 3));
    const x = (index) => 12 + index * 36;
    const y = (value) => 100 - value * .82;
    const points = (values) => values.map((value, index) => `${x(index)},${y(value)}`).join(" ");
    const area = `${x(0)},102 ${points(vals)} ${x(vals.length - 1)},102`;
    return `<svg viewBox="0 0 456 108" role="img" aria-label="Passenger demand trend"><defs><linearGradient id="chartFill" x1="0" x2="0" y1="0" y2="1"><stop offset="0" stop-color="#4de0c2" stop-opacity=".25"/><stop offset="1" stop-color="#4de0c2" stop-opacity="0"/></linearGradient></defs><path d="M12 25H444M12 53H444M12 81H444" stroke="#1b3444" stroke-width="1"/><polyline points="${points(previous)}" fill="none" stroke="#527cba" stroke-width="1.8" stroke-dasharray="4 4" opacity=".8"/><polygon points="${area}" fill="url(#chartFill)"/><polyline points="${points(vals)}" fill="none" stroke="#55dfc2" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round"/>${vals.map((value, index) => `<circle cx="${x(index)}" cy="${y(value)}" r="${index === vals.length - 1 ? 3 : 1.8}" fill="${index === vals.length - 1 ? "#b3fff0" : "#4ed8bb"}"/>`).join("")}</svg>`;
  }

  function renderTraffic() {
    $("#traffic-chart").innerHTML = chartSvg(currentRange);
    const base = currentRange === "7D" ? 12840 : currentRange === "30D" ? 48210 : 139600;
    $("#traffic-total").textContent = base.toLocaleString();
  }

  function renderDepartures() {
    $("#departure-list").innerHTML = state.trains.slice(0, 4).map((train) => `
      <div class="departure-row"><div class="train-token">${escapeHtml(train.id.slice(-3))}</div>
      <div class="departure-info"><b>${escapeHtml(train.name)}</b><small>${escapeHtml(train.from)} <span>→</span> ${escapeHtml(train.to)} · ${train.number}</small></div>
      <div class="departure-time"><b>${escapeHtml(train.depart)}</b><small>${freeSeatLabels(train).length} seats left</small></div></div>`).join("");
  }

  function activityMarkup(event, compact = false) {
    const icon = event.type === "CANCEL" ? "×" : event.type === "WAITLIST" ? "◷" : event.type === "PROMOTE" ? "↗" : event.type === "SYSTEM" ? "⌘" : "＋";
    return `<div class="${compact ? "activity-row" : "full-activity-row"}"><span class="activity-icon">${icon}</span>${compact ? `<div class="activity-copy"><b>${escapeHtml(event.message)}</b><small>${escapeHtml(event.detail || event.type)}</small></div><time>${shortTime(event.at)}</time>` : `<time>${new Date(event.at).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" })}</time><b>${escapeHtml(event.type)}</b><span>${escapeHtml(event.message)}</span><small>${escapeHtml(event.detail || shortTime(event.at))}</small>`}</div>`;
  }

  function renderOccupancy(target = "#occupancy-list") {
    $(target).innerHTML = state.trains.map((train) => {
      const capacity = allCapacity(train);
      const used = confirmedCount(train.id);
      const percent = capacity ? Math.min(100, Math.round(used / capacity * 100)) : 0;
      return `<div class="occupancy-item"><div class="occupancy-item-head"><span>${escapeHtml(train.name)}</span><small>${used} / ${capacity} seats · ${percent}%</small></div><div class="occupancy-bar"><i style="width:${percent}%"></i></div></div>`;
    }).join("");
    const allCapacityTotal = state.trains.reduce((sum, train) => sum + allCapacity(train), 0);
    const allUsed = state.trains.reduce((sum, train) => sum + confirmedCount(train.id), 0);
    if ($("#network-occupancy")) $("#network-occupancy").textContent = `${allCapacityTotal ? Math.round(allUsed / allCapacityTotal * 100) : 0}%`;
  }

  function renderOverview() {
    renderMetrics();
    renderTraffic();
    renderDepartures();
    $("#recent-activity").innerHTML = state.events.slice(0, 5).map((event) => activityMarkup(event, true)).join("") || `<div class="empty-state">No events recorded yet.</div>`;
    renderOccupancy();
    $("#nav-wait-count").textContent = state.bookings.filter((booking) => booking.status === "WAITLISTED").length;
  }

  function statusBadge(status) {
    return `<span class="status-pill status-${status.toLowerCase()}">${escapeHtml(status)}</span>`;
  }
  function categoryBadge(category) {
    return `<span class="category-pill category-${escapeHtml(category)}">${escapeHtml(category)}</span>`;
  }
  function bookingRow(booking, full = false) {
    const train = trainFor(booking.trainId) || { name: "Unknown train", from: "—", to: "—" };
    const seat = booking.seatLabels.length ? `${booking.coachId} · ${booking.seatLabels.join(", ")}` : "Waitlist";
    return `<tr><td><div class="passenger-cell"><b>${escapeHtml(booking.pnr)}</b><small>${escapeHtml(booking.passenger)}</small></div></td><td>${escapeHtml(train.id)} · ${escapeHtml(train.name)}${full ? `<br><small>${escapeHtml(train.from)} → ${escapeHtml(train.to)}</small>` : ""}</td><td>${escapeHtml(seat)}</td><td>${categoryBadge(booking.category)}</td><td>${statusBadge(booking.status)}</td><td>${formatMoney(booking.fare)}${booking.refund ? `<br><small class="refund-text">Refund ${formatMoney(booking.refund)}</small>` : ""}</td>${full ? `<td>${booking.status === "CANCELLED" ? "—" : `<button class="action-link" data-cancel-pnr="${escapeHtml(booking.pnr)}">Cancel</button>`}</td>` : ""}</tr>`;
  }

  function renderReservations() {
    const selector = $("#booking-train");
    const selected = selector.value;
    selector.innerHTML = state.trains.map((train) => `<option value="${escapeHtml(train.id)}">${escapeHtml(train.id)} · ${escapeHtml(train.name)}</option>`).join("");
    if (state.trains.some((train) => train.id === selected)) selector.value = selected;
    updateRoutePreview();
    $("#reservation-table").innerHTML = latestBookings().slice(0, 6).map((booking) => bookingRow(booking)).join("") || `<tr><td colspan="6"><div class="empty-state">No reservations yet</div></td></tr>`;
  }

  function renderBookings() {
    const query = ($("#booking-search").value || "").trim().toLowerCase();
    const status = $("#booking-status-filter").value;
    const category = $("#booking-priority-filter").value;
    const rows = latestBookings().filter((booking) => {
      const matchesQuery = !query || `${booking.pnr} ${booking.passenger} ${booking.trainId}`.toLowerCase().includes(query);
      return matchesQuery && (status === "ALL" || booking.status === status) && (category === "ALL" || booking.category === category);
    });
    $("#all-bookings-table").innerHTML = rows.map((booking) => bookingRow(booking, true)).join("") || `<tr><td colspan="7"><div class="empty-state"><b>No matching journeys</b>Try a different search or filter.</div></td></tr>`;
    $("#bookings-foot").textContent = `Showing ${rows.length} of ${state.bookings.length} reservation records`;
  }

  function renderTrains() {
    const total = state.trains.reduce((sum, train) => sum + allCapacity(train), 0);
    const occupied = state.trains.reduce((sum, train) => sum + confirmedCount(train.id), 0);
    const waiting = state.bookings.filter((booking) => booking.status === "WAITLISTED").length;
    $("#network-summary").innerHTML = [
      ["ACTIVE ROUTES", state.trains.length.toString().padStart(2, "0")],
      ["AVAILABLE SEATS", Math.max(0, total - occupied).toString()],
      ["WAITING PASSENGERS", waiting.toString().padStart(2, "0")]
    ].map(([label, value]) => `<div class="network-summary-item"><small>${label}</small><b>${value}</b></div>`).join("");
    $("#train-grid").innerHTML = state.trains.map((train) => {
      const capacity = allCapacity(train);
      const occupiedSeats = confirmedCount(train.id);
      const percent = capacity ? Math.round(occupiedSeats / capacity * 100) : 0;
      return `<article class="train-card"><div class="train-card-top"><div><span class="train-code">${escapeHtml(train.id)} · ${train.number}</span><h3>${escapeHtml(train.name)}</h3><p>${train.coaches.map((coach) => escapeHtml(coach.id)).join(" · ")} coaches</p></div><span class="live-pill"><i></i> ACTIVE</span></div><div class="train-route-line"><div><b>${escapeHtml(train.fromCode)}</b><small>${escapeHtml(train.depart)}</small></div><i class="route-dash"></i><div><b>${escapeHtml(train.toCode)}</b><small>${escapeHtml(train.arrive)}</small></div></div><div class="train-card-bottom"><span>SEATS REMAINING</span><b>${Math.max(0, capacity - occupiedSeats)} / ${capacity}</b></div><div class="occupancy-bar"><i style="width:${percent}%"></i></div><div class="train-card-bottom"><span>Occupancy</span><b>${percent}%</b></div><button class="button button-quiet button-small train-book-button" data-train-book="${escapeHtml(train.id)}">View seats &amp; reserve →</button></article>`;
    }).join("");
  }

  function renderAnalytics() {
    renderMetrics("#analytics-metrics");
    renderOccupancy("#analytics-occupancy");
    const categories = ["GENERAL", "LADIES", "SENIOR", "TATKAL"];
    const active = state.bookings.filter((booking) => booking.status !== "CANCELLED");
    $("#donut-total").textContent = String(active.length);
    $("#donut-legend").innerHTML = categories.map((category) => {
      const count = active.filter((booking) => booking.category === category).length;
      return `<span><i style="background:${CATEGORY_COLORS[category]}"></i>${category[0] + category.slice(1).toLowerCase()} <b>${count}</b></span>`;
    }).join("");
    const max = Math.max(1, ...categories.map((category) => active.filter((booking) => booking.category === category).length));
    $("#priority-chart").innerHTML = [...categories].reverse().map((category) => {
      const count = active.filter((booking) => booking.category === category).length;
      const width = Math.round(count / max * 100);
      return `<div class="priority-row"><span>${category}</span><i><b style="width:${width}%;background:${CATEGORY_COLORS[category]}"></b></i><span>${count}</span></div>`;
    }).join("");
  }

  function renderActivity() {
    const filter = $("#activity-filter").value;
    const entries = state.events.filter((event) => filter === "ALL" || event.type.includes(filter));
    $("#full-activity").innerHTML = entries.map((event) => activityMarkup(event)).join("") || `<div class="empty-state"><b>No matching activity</b>Events will appear here as you use the app.</div>`;
  }

  function updateRoutePreview() {
    const train = trainFor($("#booking-train").value);
    if (!train) return;
    $("#reservation-route").innerHTML = `<div class="route-stop"><i></i><b>${escapeHtml(train.fromCode)}</b><small>${escapeHtml(train.from)}</small></div><div class="route-stop"><i></i><b>${escapeHtml(train.toCode)}</b><small>${escapeHtml(train.to)}</small></div>`;
    $("#preview-fare").textContent = formatMoney(train.fare);
    $("#preview-availability").textContent = `${freeSeatLabels(train).length} seats open`;
  }

  function assignSeats(train, count, preferred = null) {
    const free = freeSeatLabels(train);
    if (free.length < count) return [];
    if (preferred && preferred.trainId === train.id && free.includes(preferred.label)) {
      return [preferred.label, ...free.filter((label) => label !== preferred.label)].slice(0, count);
    }
    return free.slice(0, count);
  }

  function bookJourney({ trainId, passenger, seats, category, preferred }) {
    const train = trainFor(trainId);
    const count = Number(seats);
    if (!train || !passenger.trim() || !Number.isInteger(count) || count < 1 || count > 4) {
      throw new Error("Please provide a passenger, a valid train, and 1–4 seats.");
    }
    const sequence = state.sequence++;
    const labels = assignSeats(train, count, preferred);
    const confirmed = labels.length === count;
    const first = labels[0] || "";
    const coachId = confirmed ? first.split("-")[0] : "";
    const booking = {
      pnr: `RS${String(sequence).padStart(8, "0")}`, sequence, trainId,
      passenger: passenger.trim(), category, seats: count, coachId,
      seatLabels: labels, status: confirmed ? "CONFIRMED" : "WAITLISTED",
      fare: train.fare * count, refund: 0, createdAt: timestamp(), cancelledAt: null
    };
    state.bookings.push(booking);
    addEvent(confirmed ? "BOOKING" : "WAITLIST",
      confirmed ? `${booking.passenger} confirmed on ${train.name}` : `${booking.passenger} joined the ${category} priority queue`,
      `${booking.pnr}${labels.length ? ` · ${labels.join(", ")}` : ""}`);
    return booking;
  }

  function promoteWaiting(trainId) {
    const train = trainFor(trainId);
    if (!train) return [];
    const waiting = sortWaiting(liveBookings(trainId, "WAITLISTED"));
    const promoted = [];
    for (const booking of waiting) {
      const labels = assignSeats(train, booking.seats);
      if (labels.length !== booking.seats) continue;
      booking.status = "CONFIRMED";
      booking.seatLabels = labels;
      booking.coachId = labels[0].split("-")[0];
      booking.promotedAt = timestamp();
      promoted.push(booking);
      addEvent("PROMOTE", `${booking.passenger} promoted from the waitlist`, `${booking.pnr} · ${labels.join(", ")}`);
    }
    return promoted;
  }

  function cancelBooking(pnr) {
    const booking = state.bookings.find((item) => item.pnr === pnr);
    if (!booking || booking.status === "CANCELLED") throw new Error("That booking was not found or is already cancelled.");
    const train = trainFor(booking.trainId);
    booking.refund = booking.status === "CONFIRMED" ? Math.round(booking.fare * .8) : 0;
    booking.status = "CANCELLED";
    booking.cancelledAt = timestamp();
    booking.seatLabels = [];
    booking.coachId = "";
    addEvent("CANCEL", `${booking.passenger} cancelled their reservation`, `${booking.pnr} · refund ${formatMoney(booking.refund)}`);
    const promoted = train ? promoteWaiting(train.id) : [];
    saveState();
    return { booking, promoted };
  }

  function renderSeatDialog(trainId) {
    const train = trainFor(trainId);
    if (!train) return;
    const used = usedSeatLabels(trainId);
    const wait = sortWaiting(liveBookings(trainId, "WAITLISTED"));
    const wrap = document.createElement("dialog");
    wrap.className = "seat-dialog";
    wrap.innerHTML = `<form method="dialog" class="seat-dialog-inner"><button class="dialog-close" aria-label="Close">×</button><p class="card-kicker">${escapeHtml(train.id)} · LIVE COACH MAP</p><h2>${escapeHtml(train.name)}</h2><p class="dialog-subtitle">${escapeHtml(train.from)} → ${escapeHtml(train.to)} · ${escapeHtml(train.depart)} departure</p><div class="seat-legend"><span><i class="seat-free"></i> Available</span><span><i class="seat-taken"></i> Confirmed</span><span><i class="seat-wait"></i> Waitlist</span></div><div class="dialog-coaches">${train.coaches.map((coach) => `<section><div class="coach-title"><b>${escapeHtml(coach.id)}</b><small>${escapeHtml(coach.type)} · ${coach.seats} seats</small></div><div class="seat-grid">${Array.from({ length: coach.seats }, (_, index) => { const label = `${coach.id}-${String(index + 1).padStart(2, "0")}`; const booked = used.has(label); return `<button type="button" class="seat-chip ${booked ? "taken" : "free"}" ${booked ? "disabled" : `data-quick-seat="${escapeHtml(train.id)}" data-seat-label="${escapeHtml(label)}"`} title="${booked ? "Confirmed seat" : `Reserve ${escapeHtml(label)}`}">${String(index + 1).padStart(2, "0")}</button>`; }).join("")}</div></section>`).join("")}</div><div class="dialog-waitlist"><b>Priority queue</b><span>${wait.length} waiting</span></div><div class="dialog-wait-list">${wait.slice(0, 4).map((booking, index) => `<span><i>${index + 1}</i>${escapeHtml(booking.passenger)} <small>${escapeHtml(booking.category)}</small></span>`).join("") || "<small>No passengers waiting</small>"}</div><button class="button button-primary button-wide" value="close">Done</button></form>`;
    document.body.append(wrap);
    wrap.addEventListener("click", (event) => {
      if (event.target === wrap) wrap.close();
      const button = event.target.closest("[data-quick-seat]");
      if (button) {
        wrap.close();
        setView("reservations");
        $("#booking-train").value = trainId;
        updateRoutePreview();
        $("#booking-form [name=seats]").value = "1";
        preferredSeat = { trainId, label: button.dataset.seatLabel };
        updateSeatPickerLabel();
        $("#booking-form [name=passenger]").focus();
        toast(`Select passenger details to reserve ${preferredSeat.label}.`);
      }
    });
    wrap.addEventListener("close", () => wrap.remove(), { once: true });
    wrap.showModal();
  }

  function renderLabTrace(threadCount, capacity, mode) {
    const target = $("#trace-track");
    const shown = Math.min(12, threadCount);
    const confirmedSample = Math.round(shown * Math.min(threadCount, capacity) / threadCount);
    target.innerHTML = `<div class="thread-lanes ${threadCount > 16 ? "compact" : ""}">${Array.from({ length: shown }, (_, i) => { const queued = i >= confirmedSample; return `<div class="thread-lane"><span>THREAD ${String(i + 1).padStart(2, "0")}</span><div class="lane-track"><i class="lane-bar ${mode === "unsafe" ? "unsafe" : queued ? "queued" : ""}" style="--lane-width:${35 + ((i * 19) % 65)}%;animation-delay:${i * 35}ms"></i></div><span class="lane-time">${queued ? "QUEUED" : "LOCKED"}</span></div>`; }).join("")}</div>`;
  }

  async function runSimulation() {
    if (simulationRunning) return;
    simulationRunning = true;
    const threadCount = Number($("#thread-range").value);
    const capacity = Number($("#lab-capacity").value);
    const mode = $("#lab-mode").value;
    const start = performance.now();
    $("#lab-run-state").textContent = "RUNNING";
    $("#lab-run-state").className = "run-badge running";
    $("#run-lab").disabled = true;
    renderLabTrace(threadCount, capacity, mode);
    await new Promise((resolve) => window.setTimeout(resolve, 750));
    const confirmed = Math.min(threadCount, capacity);
    const waitlisted = Math.max(0, threadCount - capacity);
    const duplicateSeats = mode === "unsafe" && threadCount > 1 ? Math.min(threadCount - 1, Math.ceil(threadCount * .18)) : 0;
    const elapsed = Math.max(8, Math.round(performance.now() - start));
    const values = [confirmed, waitlisted, duplicateSeats, `${elapsed} ms`];
    $$("#lab-metrics b").forEach((item, index) => { item.textContent = values[index]; });
    $("#lab-run-state").textContent = "COMPLETE";
    $("#lab-run-state").className = "run-badge done";
    $("#run-lab").disabled = false;
    simulationRunning = false;
    addEvent("SYSTEM", `${mode === "safe" ? "Safe" : "Unsafe"} concurrency simulation completed`,
      `${threadCount} workers · ${confirmed} confirmed · ${duplicateSeats} duplicate seat attempts`);
    toast(mode === "safe" ? "Safe run complete — no duplicate seats detected." : "Unsafe demo complete — stale reads produced collisions.");
  }

  function makeSvgReports() {
    const occupancy = state.trains.map((train, index) => {
      const percent = Math.round(confirmedCount(train.id) / Math.max(1, allCapacity(train)) * 100);
      const y = 82 + index * 54;
      return `<text x="30" y="${y}" fill="#b9c9d0" font-size="13">${escapeHtml(train.id)} · ${escapeHtml(train.name)}</text><rect x="280" y="${y - 15}" width="390" height="17" rx="8" fill="#203545"/><rect x="280" y="${y - 15}" width="${percent * 3.9}" height="17" rx="8" fill="#52d9bd"/><text x="690" y="${y}" fill="#52d9bd" font-size="12">${percent}%</text>`;
    }).join("");
    const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="760" height="${120 + state.trains.length * 54}" viewBox="0 0 760 ${120 + state.trains.length * 54}"><rect width="100%" height="100%" fill="#0c1a28"/><text x="30" y="42" fill="#eaf4f5" font-size="22" font-family="Arial">RailSetu · Live seat occupancy</text>${occupancy}</svg>`;
    const stats = countMetrics();
    const html = `<!doctype html><html><head><meta charset="utf-8"><title>RailSetu network report</title><style>body{margin:0;padding:40px;background:#081522;color:#e8f1f5;font:15px system-ui}main{max-width:980px;margin:auto}.cards{display:flex;gap:12px;flex-wrap:wrap}.card{padding:18px 22px;background:#102334;border:1px solid #244251;border-radius:12px;min-width:145px}.card small{display:block;color:#89a1ae}.card b{display:block;margin-top:8px;font-size:26px}img{width:100%;margin-top:24px;border:1px solid #244251;border-radius:12px}</style></head><body><main><p>RAILSETU · LOCAL OPERATIONS REPORT</p><h1>Network snapshot</h1><div class="cards"><div class="card"><small>Active routes</small><b>${stats.trains}</b></div><div class="card"><small>Confirmed</small><b>${stats.confirmed}</b></div><div class="card"><small>Waitlisted</small><b>${stats.waiting}</b></div><div class="card"><small>Seat utilization</small><b>${stats.utilization}%</b></div></div><img src="occupancy.svg" alt="Seat occupancy chart"><p>Priority scheduling: Senior → Ladies → Tatkal → General. FIFO is preserved within each priority tier.</p></main></body></html>`;
    return { svg, html };
  }

  function download(name, content, type) {
    const blob = new Blob([content], { type });
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement("a");
    anchor.href = url; anchor.download = name; anchor.click();
    URL.revokeObjectURL(url);
  }

  function exportData() {
    const header = ["PNR", "Passenger", "Train", "Category", "Status", "Coach", "Seats", "Fare INR", "Refund INR"];
    const rows = latestBookings().map((booking) => [booking.pnr, booking.passenger, booking.trainId, booking.category, booking.status, booking.coachId, booking.seatLabels.join("|"), booking.fare, booking.refund]);
    const csv = [header, ...rows].map((row) => row.map((field) => `"${String(field).replaceAll('"', '""')}"`).join(",")).join("\r\n");
    download("railsetu-bookings.csv", csv, "text/csv;charset=utf-8");
    const reports = makeSvgReports();
    download("railsetu-occupancy.svg", reports.svg, "image/svg+xml");
    download("railsetu-report.html", reports.html, "text/html;charset=utf-8");
    const report = `# RailSetu Browser Report\n\n- Routes: ${state.trains.length}\n- Confirmed: ${countMetrics().confirmed}\n- Waitlisted: ${countMetrics().waiting}\n- Seat utilization: ${countMetrics().utilization}%\n- Captured: ${timestamp()}\n`;
    download("railsetu-report.md", report, "text/markdown;charset=utf-8");
    addEvent("SYSTEM", "Browser reports exported", "CSV · SVG · HTML · Markdown");
    toast("Reports downloaded: CSV, SVG, HTML, and Markdown.");
  }

  async function resetDemo() {
    const yes = await askConfirm("Reset local demo data?", "This clears browser-only reservations and restores the original sample network.", "Reset data");
    if (!yes) return;
    state = makeSeed();
    preferredSeat = null;
    updateSeatPickerLabel();
    saveState();
    renderAll();
    toast("Demo data restored.");
  }

  function renderAll() {
    renderOverview();
    renderReservations();
    renderBookings();
    renderTrains();
    renderAnalytics();
    renderActivity();
  }

  document.addEventListener("click", async (event) => {
    const viewLink = event.target.closest("[data-view-link]");
    if (viewLink) { event.preventDefault(); setView(viewLink.dataset.viewLink); return; }
    const nav = event.target.closest("[data-view]");
    if (nav) { setView(nav.dataset.view); return; }
    const action = event.target.closest("[data-action]");
    if (action) {
      if (action.dataset.action === "new-booking") { setView("reservations"); $("#booking-form [name=passenger]").focus(); }
      if (action.dataset.action === "choose-seat") renderSeatDialog($("#booking-train").value);
      if (action.dataset.action === "export") exportData();
      if (action.dataset.action === "reset") await resetDemo();
      return;
    }
    const cancelButton = event.target.closest("[data-cancel-pnr]");
    if (cancelButton) {
      const pnr = cancelButton.dataset.cancelPnr;
      if (!await askConfirm("Cancel this journey?", `Cancel ${pnr}? A confirmed booking receives an 80% fare refund.`, "Cancel journey")) return;
      try {
        const result = cancelBooking(pnr);
        renderAll();
        toast(result.promoted.length ? `Booking cancelled. ${result.promoted.length} passenger(s) promoted.` : `Booking cancelled · refund ${formatMoney(result.booking.refund)}.`);
      } catch (error) { toast(error.message, "error"); }
      return;
    }
    const trainButton = event.target.closest("[data-train-book]");
    if (trainButton) { renderSeatDialog(trainButton.dataset.trainBook); return; }
    const range = event.target.closest("[data-chart-range]");
    if (range) {
      currentRange = range.dataset.chartRange;
      $$(".segmented button").forEach((button) => button.classList.toggle("selected", button === range));
      renderTraffic();
    }
  });

  $("#booking-form").addEventListener("submit", (event) => {
    event.preventDefault();
    const data = new FormData(event.currentTarget);
    try {
      const booking = bookJourney({
        trainId: data.get("train"), passenger: data.get("passenger"),
        seats: data.get("seats"), category: data.get("category"), preferred: preferredSeat
      });
      preferredSeat = null;
      updateSeatPickerLabel();
      event.currentTarget.reset();
      renderAll();
      setView("bookings");
      toast(booking.status === "CONFIRMED"
        ? `${booking.pnr} confirmed · ${booking.seatLabels.join(", ")}`
        : `${booking.pnr} added to the ${booking.category} queue.`);
    } catch (error) { toast(error.message, "error"); }
  });

  $("#booking-train").addEventListener("change", () => { preferredSeat = null; updateSeatPickerLabel(); updateRoutePreview(); });
  $("#booking-search").addEventListener("input", renderBookings);
  $("#booking-status-filter").addEventListener("change", renderBookings);
  $("#booking-priority-filter").addEventListener("change", renderBookings);
  $("#activity-filter").addEventListener("change", renderActivity);
  $("#thread-range").addEventListener("input", (event) => { $("#thread-output").value = `${event.target.value} threads`; });
  $("#run-lab").addEventListener("click", runSimulation);
  $("#deadlock-demo").addEventListener("click", () => {
    $("#deadlock-result").textContent = "Acquiring both locks with an atomic ordering routine…";
    $("#deadlock-result").classList.add("working");
    window.setTimeout(() => {
      $("#deadlock-result").textContent = "✓ Both workers completed. Circular wait prevented by consistent lock ordering.";
      $("#deadlock-result").classList.remove("working");
      addEvent("SYSTEM", "Deadlock prevention demo completed", "Consistent lock ordering");
      toast("Deadlock prevention demonstration completed.");
    }, 450);
  });

  $("#global-search").addEventListener("input", (event) => {
    const value = event.target.value.trim().toLowerCase();
    if (value.length < 2) return;
    const match = latestBookings().find((booking) =>
      `${booking.pnr} ${booking.passenger} ${booking.trainId}`.toLowerCase().includes(value));
    if (match) {
      setView("bookings");
      $("#booking-search").value = event.target.value;
      renderBookings();
    }
  });

  document.addEventListener("keydown", (event) => {
    if ((event.metaKey || event.ctrlKey) && event.key.toLowerCase() === "k") {
      event.preventDefault();
      $("#global-search").focus();
    }
    if (event.key === "Escape" && event.target === document.body) setView("overview");
  });

  renderAll();
})();
